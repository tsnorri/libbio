/*
 * Copyright (c) 2023-2026 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <algorithm>
#include <atomic>
#include <boost/context/fiber.hpp>
#include <condition_variable>
#include <cerrno>
#include <chrono>
#include <cmath>				// std::floor
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <libbio/assert.hh>
#include <libbio/dispatch.hh>
#include <libbio/dispatch/detail/task_executor.hh>
#include <libbio/dispatch/queue.hh>
#include <libbio/dispatch/thread_pool.hh>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <signal.h>				// sigfillset, ::pthread_sigmask
#include <stdexcept>
#include <sys/signal.h>
#include <thread>
#include <vector>

namespace chrono	= std::chrono;


namespace {
	libbio::dispatch::detail::task_executor *make_new_task_executor(bool should_use_fibers)
	{
#if LIBBIO_ENABLE_DISPATCH_FIBER_SUPPORT
		if (should_use_fibers)
			return new libbio::dispatch::detail::fiber_task_executor{};
		else
			return new libbio::dispatch::detail::direct_task_executor{};
#else
		libbio_assert(!should_use_fibers);
		return new libbio::dispatch::detail::direct_task_executor{};
#endif
	}
}


namespace libbio::dispatch {

	thread_pool::thread_count_type const thread_pool::default_max_worker_threads = thread_pool::thread_count_type(
		std::floor(1.5 * (std::thread::hardware_concurrency() ?: 2))
	);


	void block_signals()
	{
		// Block all signals.
		sigset_t mask{};
		if (-1 == sigfillset(&mask))
			throw std::runtime_error(::strerror(errno));
		if (-1 == ::pthread_sigmask(SIG_SETMASK, &mask, nullptr))
			throw std::runtime_error(::strerror(errno));
	}


	class worker_thread_runner
	{
	private:
		typedef	chrono::steady_clock					clock_type;
		typedef clock_type::time_point					time_point_type;
		typedef clock_type::duration					duration_type;
		typedef detail::task_executor::queue_item_type	queue_item_type;
		typedef detail::task_executor::pool_lock_type	pool_lock_type;
		typedef std::int64_t							task_count_type;

	private:
		thread_pool		*m_thread_pool{};
		duration_type	m_max_idle_time{};
		bool			m_uses_fiber_executor{};

	private:
		bool handle_barrier(queue_item_type &queue_item, pool_lock_type &pool_lock, task_count_type const executed_tasks);

	public:
		worker_thread_runner(thread_pool &pool, duration_type const max_idle_time, bool uses_fiber_executor):
			m_thread_pool{&pool},
			m_max_idle_time{max_idle_time},
			m_uses_fiber_executor{uses_fiber_executor}
		{
		}

		void run();
		void operator()() { run(); }

		void remove_from_pool(task_count_type const executed_tasks);
		void begin_idle(task_count_type const executed_tasks);
	};


	void worker_thread_runner::remove_from_pool(task_count_type const executed_tasks)
	{
		auto &pool{*m_thread_pool};
		pool.m_waiting_tasks -= executed_tasks;
		pool.remove_worker();
	}


	void worker_thread_runner::begin_idle(task_count_type const executed_tasks)
	{
		auto &pool{*m_thread_pool};
		pool.m_waiting_tasks -= executed_tasks;
		++pool.m_idle_workers;
	}


	void worker_thread_runner::run()
	{
		block_signals();

		libbio_assert(m_thread_pool);
		auto &pool{*m_thread_pool};

		auto last_wake_up_time{clock_type::now()};

		// Prepare the task executor.
		std::unique_ptr <detail::task_executor> task_executor{make_new_task_executor(m_uses_fiber_executor)};
		task_executor->prepare(pool);
		task_executor->assign_thread_executor();

		// task_executor maintains a std::unique_lock that wraps pool.m_mutex.

		{
			while (true)
			{
				task_count_type executed_tasks{}; // Total over the iterations of the loop below (but not the enclosing loop).

				{
					// Critical section 1.
					// We need the queues to persist while tasks are being executed.
					// Acquiring m_queue_mutex is sufficient b.c. the queue will not get deallocated before
					// it has been removed from the thread pool.
					std::shared_lock const queue_lock{pool.m_queue_mutex};
					while (true)
					{
						auto const prev_executed_tasks{executed_tasks};
						for (auto queue : pool.m_queues)
						{
							task_executor->check_pending_and_run();

							if (queue->m_task_queue.try_dequeue(task_executor->current_queue_item()))
							{
								++executed_tasks;

								if (!handle_barrier(task_executor->current_queue_item(), task_executor->pool_lock(), executed_tasks))
									return;

								task_executor->run();
							}
						} // Queue loop

						if (executed_tasks == prev_executed_tasks)
							break;
					} // Inner while (true)
				} // Critical section 1

				{
					// Check the last wake-up time.
					auto const now{clock_type::now()};
					auto const diff{now - last_wake_up_time};
					if (0 == executed_tasks && m_max_idle_time <= diff && !task_executor->has_pending_tasks())
					{
						// Critical section 2.
						task_executor->pool_lock().lock();
						remove_from_pool(executed_tasks); // zero but does not matter.
						return; // Destroys task_executor and thus unlocks.
					}

					last_wake_up_time = now;
				}

				// Critical section 2.
				{
					if (!task_executor->lock_and_check_pending_and_run())
						continue;

					// Acquired the lock since lock_and_check_pending_and_run() returned true.
					// Handle spurious wake-ups by repeatedly calling wait_for().
					while (true)
					{
						// m_mutex is locked when wait_until() returns.
						switch (pool.m_cv.wait_for(task_executor->pool_lock(), m_max_idle_time))
						{
							case std::cv_status::no_timeout:
								break;

							case std::cv_status::timeout:
								// Still marked idle.
								pool.remove_idle_worker();
								return;
						}

						if (!pool.m_should_continue)
						{
							pool.remove_idle_worker();
							return;
						}

						// Check for a spurious wake-up.
						if (pool.m_notified_workers)
						{
							// Intentional wake-up.
							--pool.m_notified_workers;
							break;
						}
					}

					task_executor->pool_lock().unlock();
				}
			} // Outer while (true)
		}
	}


	bool worker_thread_runner::handle_barrier(
		queue_item_type &queue_item,
		pool_lock_type &pool_lock,
		task_count_type const executed_tasks
	)
	{
#if LIBBIO_ENABLE_DISPATCH_BARRIER
		libbio_assert(queue_item.barrier_);
		auto &pool{*m_thread_pool};
		auto &bb{*queue_item.barrier_};
		barrier::status_underlying_type state{barrier::NOT_EXECUTED};
		if (bb.m_state.compare_exchange_strong(state, barrier::EXECUTING, std::memory_order_acq_rel, std::memory_order_acquire))
		{
			// Wait for the previous tasks and the previous barrier to complete.
			bb.m_previous_has_finished.wait(false, std::memory_order_acquire);

			bb.m_task();
			bb.m_task = task{}; // Deallocate memory.

			bool should_continue{};

			{
				std::lock_guard const lock_{pool_lock};
				should_continue = pool.m_should_continue;
				if (!should_continue)
					remove_from_pool(executed_tasks);
			}

			if (should_continue)
			{
				bb.m_state.store(barrier::DONE, std::memory_order_release);
				bb.m_state.notify_all();
			}
			else
			{
				bb.m_state.store(barrier::DO_STOP, std::memory_order_release);
				bb.m_state.notify_all();
				return false;
			}
		}
		else
		{
			// The barrier task is either currently being executed or has already been finished.
			switch (state)
			{
				case barrier::EXECUTING:
				{
					bb.m_state.wait(barrier::EXECUTING, std::memory_order::acquire);
					// The acquire operation above should make the modification visible here.
					if (barrier::DO_STOP == bb.m_state.load(std::memory_order_relaxed))
					{
						std::lock_guard const lock_{pool_lock};
						remove_from_pool(executed_tasks);
						return false;
					}

					break;
				}

				case barrier::DONE:
					break;

				// Stop if the barrier’s task called m_pool.stop().
				case barrier::DO_STOP:
				{
					std::lock_guard const lock_{pool_lock};
					remove_from_pool(executed_tasks);
					return false;
				}

				case barrier::NOT_EXECUTED:
					// Unexpected.
					std::abort();
			}
		}
#endif
		return true;
	}


	void thread_pool::add_queue(parallel_queue &queue)
	{
		std::lock_guard const lock{m_queue_mutex}; // Exclusive
		m_queues.push_back(&queue);
	}


	void thread_pool::remove_queue(parallel_queue const &queue)
	{
		std::lock_guard const lock{m_queue_mutex}; // Exclusive
		auto const it{std::find(m_queues.begin(), m_queues.end(), &queue)};
		if (it == m_queues.end())
			return;

		m_queues.erase(it);
	}


	void thread_pool::start_worker_()
	{
		++m_current_workers;
		std::thread thread{worker_thread_runner{*this, m_max_idle_time, m_uses_fiber_executor}};
		thread.detach();
	}


	void thread_pool::remove_worker()
	{
		libbio_assert_lt(0, m_current_workers);
		if (0 == --m_current_workers)
			m_stop_cv.notify_one();
	}


	void thread_pool::remove_idle_worker()
	{
		// Worker still marked idle.
		libbio_assert_lt(0, m_current_workers);
		if (0 == --m_current_workers)
			m_stop_cv.notify_one();
	}


	void thread_pool::start_worker()
	{
		std::lock_guard const lock{m_mutex};
		start_worker_();
	}


	void thread_pool::notify()
	{
		{
			std::lock_guard const lock{m_mutex};
			++m_waiting_tasks;
			if (m_idle_workers)
			{
				--m_idle_workers;
				++m_notified_workers;
				goto do_notify;
			}

			if (m_max_workers <= m_current_workers && m_min_workers <= m_current_workers)
				return;

			// Can start a new thread.
			start_worker_();
		}

	do_notify:
		m_cv.notify_one();
	}


	void thread_pool::stop(bool should_wait)
	{
		{
			std::lock_guard lock{m_mutex};
			m_should_continue = false;
		}

		m_cv.notify_all();

		if (should_wait)
			wait();
	}


	void thread_pool::wait()
	{
		std::unique_lock lock{m_mutex};
		if (0 < m_current_workers)
			m_stop_cv.wait(lock, [this]{ return 0 == m_current_workers; });
	}
}

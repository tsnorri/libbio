/*
 * Copyright (c) 2023-2026 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_DISPATCH_THREAD_POOL_HH
#define LIBBIO_DISPATCH_THREAD_POOL_HH

#include <chrono>					// std::chrono::steady_clock etc.
#include <condition_variable>
#include <cstdint>
#include <libbio/assert.hh>
#include <libbio/dispatch/fwd.hh>
#include <mutex>
#include <shared_mutex>
#include <vector>


namespace libbio::dispatch::detail {
	class task_executor;
	class fiber_task_executor;
}


namespace libbio::dispatch {

	void block_signals();


	class thread_pool
	{
		friend class worker_thread_runner;
		friend class detail::task_executor;
		friend class detail::fiber_task_executor;

	public:
		typedef std::uint32_t thread_count_type;
		constexpr static inline auto const default_max_idle_time{std::chrono::seconds(15)};
		static thread_count_type const default_max_worker_threads;

	private:
		typedef std::chrono::steady_clock	clock_type;
		typedef clock_type::duration		duration_type;

	private:
		std::vector <parallel_queue *>	m_queues;									// Non-owning.
		std::int64_t					m_waiting_tasks{};
		duration_type					m_max_idle_time{default_max_idle_time};
		thread_count_type				m_max_workers{default_max_worker_threads};
		thread_count_type				m_min_workers{};							// Overrides m_max_workers if greater.
		thread_count_type				m_current_workers{};
		thread_count_type				m_idle_workers{};
		thread_count_type				m_notified_workers{};						// For detecting spurious wake-ups.
		std::condition_variable			m_cv{};										// For pausing the workers.
		std::condition_variable			m_stop_cv{};								// For stopping the thread pool.
		std::shared_mutex				m_queue_mutex{};							// Protects m_queues
		std::mutex						m_mutex{};									// Protects m_waiting_tasks, m_current_workers, m_idle_workers, m_should_continue.
		bool							m_should_continue{true};
		bool							m_uses_fiber_executor{};

	private:
		void start_worker_();
		void remove_worker();
		void remove_idle_worker();

		std::condition_variable &condition_variable() { return m_cv; }
		std::mutex &mutex() { return m_mutex; }

	public:
		static inline thread_pool &shared_pool();

		inline void set_uses_fiber_executor(bool flag);

		void add_queue(parallel_queue &queue);			// Thread-safe.
		void remove_queue(parallel_queue const &queue);	// Thread-safe.
		void stop(bool should_wait = true);				// Thread-safe.

		thread_count_type min_workers() const { return m_min_workers; }
		thread_count_type max_workers() const { return m_max_workers; }
		void set_min_workers(thread_count_type const count) { m_min_workers = count; }
		void set_max_workers(thread_count_type const count) { m_max_workers = count; }

		void notify();									// Task was added to an observed queue. Thread-safe.
		void wait();
		void start_worker();							// Thread-safe.
		~thread_pool() { stop(); } // parallel_queue expects its thread_pool to persist until the queue has been deallocated.
	};


	thread_pool &thread_pool::shared_pool()
	{
		static thread_pool pool;
		return pool;
	}


	void thread_pool::set_uses_fiber_executor(bool flag)
	{
#if !LIBBIO_ENABLE_DISPATCH_FIBER_SUPPORT
		libbio_assert(!flag, "Not compiled with fiber support.");
#endif
		m_uses_fiber_executor = flag;
	}
}

#endif

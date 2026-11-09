/*
 * Copyright (c) 2023-2026 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <atomic>
#include <libbio/assert.hh>
#include <libbio/dispatch.hh>
#include <libbio/dispatch/detail/task_executor.hh>
#include <libbio/dispatch/task_decl.hh>
#include <mutex>
#include <utility>


namespace libbio::dispatch {

	void group::wait()
	{
		exit();

		std::unique_lock lock(m_mutex);
		m_cv.wait(lock, [this]{ return m_should_stop_waiting; });

		// Restore the group’s initial state.
		m_should_stop_waiting = false;
		m_count.fetch_add(1, std::memory_order_relaxed);
	}


	void group::wait_and_yield()
	{
		exit();
		auto *thread_task_executor{detail::task_executor::thread_executor()};

		if (thread_task_executor)
		{
			auto const should_stop_waiting{[this]{
				std::lock_guard const lock{m_mutex};
				return m_should_stop_waiting;
			}()};

			if (!should_stop_waiting)
			{
				m_task_executor = thread_task_executor;
				thread_task_executor->yield(*this);
				m_task_executor = nullptr;
			}
		}
		else
		{
			std::unique_lock lock{m_mutex};
			m_cv.wait(lock, [this]{ return m_should_stop_waiting; });
		}

		// Restore the group’s initial state.
		m_should_stop_waiting = false;
		m_count.fetch_add(1, std::memory_order_relaxed);
	}


	void group::notify(struct queue &queue, task tt)
	{
		m_queue = &queue;
		m_task = std::move(tt);
		// Relaxed should be enough b.c. exit() uses std::memory_order_acq_rel.
		m_count.fetch_or(NOTIFY_MASK, std::memory_order_relaxed);

		exit();
	}


	void group::exit()
	{
		auto const res(m_count.fetch_sub(1, std::memory_order_acq_rel));
		libbio_assert_neq(0, ~NOTIFY_MASK & res);
		if ((NOTIFY_MASK | 1) == res)
		{
			m_queue->async(std::move(m_task));
			m_queue = nullptr;
			m_task = task{};
			m_count.fetch_add(1, std::memory_order_relaxed); // Restore the group’s initial state.
		}
		else if (1 == res)
		{
			{
				std::lock_guard lock(m_mutex);
				m_should_stop_waiting = true;
			}

			// Stop waiting.
			m_cv.notify_all(); // Makes wait() continue.
			if (m_task_executor)
				m_task_executor->notify_can_resume();
		}
	}
}

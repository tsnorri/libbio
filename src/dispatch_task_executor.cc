/*
 * Copyright (c) 2026 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <libbio/assert.hh>
#include <libbio/dispatch/detail/task_executor.hh>
#include <libbio/dispatch/thread_pool.hh>
#include <mutex>
#include <utility>


namespace libbio::dispatch::detail {

	thread_local task_executor *task_executor::thread_executor_ = nullptr;


	void task_executor::run_task(queue_item_type &queue_item)
	{
		// Run the task.
		queue_item.task_();
		if (queue_item.group_)
			queue_item.group_->exit(); // Important to do only after executing the task, since it can add new tasks to the group.
	}


#if LIBBIO_ENABLE_DISPATCH_FIBER_SUPPORT
	void fiber_task_executor::prepare(thread_pool &pool)
	{
		task_executor::prepare(pool);
		m_pool = &pool;
		m_current.emplace_front();
	}


	void fiber_task_executor::run()
	{
		// Since we move list items, not task_items, the address should stay valid
		// even if the execution stops while running the lambda below.
		auto &task_item{m_current.front()};
		m_executing_item = &task_item;

		task_item.did_finish = false;
		task_item.fiber = std::move(task_item.fiber).resume_with([&task_item](boost::context::fiber &&worker_fiber){
			task_item.worker_fiber = &worker_fiber;

			// Run the task.
			run_task(task_item.queue_item);

			task_item.worker_fiber = nullptr;
			return std::move(worker_fiber).resume();
		});

		// Still valid.
		task_item.did_finish = true;

		// Finished running.
		m_executing_item = nullptr;
	}


	void fiber_task_executor::yield(group &gg)
	{
		libbio_assert(m_executing_item);
		auto &task_item{*m_executing_item};

		libbio_assert(task_item.worker_fiber); // Should be valid b.c. called from inside run().
		auto &worker_fiber{*task_item.worker_fiber};

		task_item.group_ = &gg; // Not owned by us.

		// Move to pending if needed.
		if (m_executing_item == &m_current.front())
		{
			m_pending.splice_after(m_pending.before_begin(), m_current);
			m_current.emplace_front();
		}

		m_executing_item = nullptr; // Finished running for now.
		worker_fiber = std::move(worker_fiber).resume();
	}


	void fiber_task_executor::run_pending()
	{
		// Iterate the pending task list and resume if possible.
		auto prev_it{m_pending.before_begin()};
		auto it{m_pending.begin()};
		while (m_pending.end() != it)
		{
			auto &task_item{*it};
			if (task_item.can_resume())
			{
				m_executing_item = &task_item;
				task_item.resume();
				m_executing_item = nullptr;

				if (task_item.did_finish)
				{
					it = m_pending.erase_after(prev_it);
					continue;
				}
			}

			prev_it = it;
			++it;
		}
	}


	void fiber_task_executor::check_pending_and_run()
	{
		libbio_assert(!m_executing_item);

		while (has_pending_tasks())
		{
			// Check if we have resumable tasks.
			{
				std::lock_guard const lock{m_pool_lock};
				if (!m_has_resumable_tasks)
					break;

				m_has_resumable_tasks = false;
			}

			run_pending();
		}
	}


	bool fiber_task_executor::lock_and_check_pending_and_run()
	{
		libbio_assert(!m_executing_item);

		m_pool_lock.lock();

		// Since we need to lock anyway, there is no need to check for
		// pending (possibly non-resumable) tasks.

		if (!m_has_resumable_tasks)
			return true; // Do not unlock.

		m_has_resumable_tasks = false;
		m_pool_lock.unlock();

		while (true)
		{
			run_pending();

			// Check again if we have resumable tasks.
			{
				std::lock_guard const lock{m_pool_lock};
				if (!m_has_resumable_tasks)
					break;

				m_has_resumable_tasks = false;
			}
		}

		return false;
	}


	void fiber_task_executor::notify_can_resume()
	{
		{
			std::lock_guard const lock{m_pool_lock};
			m_has_resumable_tasks = true;
		}

		// Unfortunately we have no way of notifying the specific worker.
		m_pool->condition_variable().notify_all();
	}
#endif
}

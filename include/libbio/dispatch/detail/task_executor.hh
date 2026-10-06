/*
 * Copyright (c) 2026 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_DISPATCH_TASK_EXECUTOR_HH
#define LIBBIO_DISPATCH_TASK_EXECUTOR_HH

#include <libbio/dispatch/queue.hh>
#include <libbio/dispatch/task_decl.hh>
#include <mutex>
#include <stdexcept>
#include <utility>

#ifndef LIBBIO_ENABLE_DISPATCH_FIBER_SUPPORT
#	define LIBBIO_ENABLE_DISPATCH_FIBER_SUPPORT 0
#endif

#if LIBBIO_ENABLE_DISPATCH_FIBER_SUPPORT
#	include <boost/context/fiber.hpp>
#	include <forward_list>
#endif



namespace libbio::dispatch {

	class group;
	class thread_pool;
}


namespace libbio::dispatch::detail {

	class task_executor
	{
		friend struct fiber_task_executor_item;

	public:
		typedef std::unique_lock <std::mutex> pool_lock_type;
		typedef parallel_queue::queue_item queue_item_type;

	protected:
		static thread_local task_executor *thread_executor_;
		pool_lock_type m_pool_lock{};

	protected:
		static void run_task(queue_item_type &item);

	public:
		static task_executor *thread_executor() { return thread_executor_; }
		void assign_thread_executor() { thread_executor_ = this; }

		virtual ~task_executor() {}

		pool_lock_type &pool_lock() { return m_pool_lock; }

		virtual void prepare(thread_pool &pool) { m_pool_lock = std::unique_lock{pool.mutex(), std::defer_lock}; }
		virtual queue_item_type &current_queue_item() = 0;
		virtual void run() = 0;

		virtual bool has_pending_tasks() const = 0;
		virtual void check_pending_and_run() = 0;
		virtual bool lock_and_check_pending_and_run() = 0;

		// Do not use with a barrier in the queue; things break.
		virtual void yield(group &gg) = 0;
		virtual void notify_can_resume() = 0;
	};


	class direct_task_executor final : public task_executor
	{
	private:
		queue_item_type m_queue_item;

	public:
		queue_item_type &current_queue_item() override { return m_queue_item; }
		void run() override { run_task(m_queue_item); }

		bool has_pending_tasks() const override { return false; }
		void check_pending_and_run() override {}
		bool lock_and_check_pending_and_run() override { m_pool_lock.lock(); return true; }

		void yield(group &gg) override { throw std::runtime_error("Unable to yield when running tasks directly"); }
		void notify_can_resume() override {}
	};


#if LIBBIO_ENABLE_DISPATCH_FIBER_SUPPORT
	struct fiber_task_executor_item
	{
		friend class fiber_task_executor;

	private:
		typedef task_executor::queue_item_type queue_item_type;

		boost::context::fiber fiber{};
		queue_item_type queue_item{};
		boost::context::fiber *worker_fiber{};
		group *group_{};
		bool did_finish{};

		bool can_resume() const { return 0 == group_->m_count.load(std::memory_order_acquire); }
		void resume() { fiber = std::move(fiber).resume(); }
	};


	class fiber_task_executor final : public task_executor
	{
	private:
		typedef fiber_task_executor_item task_item_type;
		typedef std::forward_list <task_item_type> task_item_list;

	private:
		task_item_list m_current;
		task_item_list m_pending;
		task_item_type *m_executing_item{};
		thread_pool *m_pool{};
		bool m_has_resumable_tasks{};

	private:
		void run_pending();

	public:
		void prepare(thread_pool &pool) override;
		queue_item_type &current_queue_item() override { return m_current.front().queue_item; }
		void run() override;

		bool has_pending_tasks() const override { return !m_pending.empty(); }
		void check_pending_and_run() override;
		bool lock_and_check_pending_and_run() override;

		void yield(group &gg) override;
		void notify_can_resume() override;
	};
#endif
}

#endif

/*
 * Copyright (c) 2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <algorithm>
#include <atomic>
#include <chrono>
#include <libbio/assert.hh>
#include <libbio/dispatch/event.hh>
#include <libbio/dispatch/events/manager.hh>
#include <libbio/dispatch/queue.hh>
#include <libbio/dispatch/task_def.hh>
#include <libbio/dispatch/thread_pool.hh>
#include <memory>
#include <mutex>
#include <sys/signal.h>
#include <sys/wait.h>							// ::waitpid()
#include <thread>
#include <utility>

namespace chrono	= std::chrono;


namespace libbio::dispatch::events
{
	void manager_base::stop_and_wait()
	{
		stop();

		// Wait until m_is_running_worker is false.
		// (Spurious unblocks are guaranteed to be handled.)
		m_is_running_worker.wait(true, std::memory_order_acquire);
	}


	void manager_base::run()
	{
		run_();

		// It does not matter even if we are not in a worker thread.
		m_is_running_worker.store(false, std::memory_order_release);
		m_is_running_worker.notify_all();
	}


	auto manager_base::check_timers() -> duration_type
	{
		duration_type next_firing_time{timer::DURATION_MAX};

		{
			std::lock_guard lock{m_timer_mutex};

			if (m_timer_entries.empty())
				return next_firing_time;

			do
			{
				auto &next_timer(m_timer_entries.front());
				if (!next_timer.timer->is_enabled())
				{
					std::pop_heap(m_timer_entries.begin(), m_timer_entries.end(), heap_cmp_type{});
					m_timer_entries.pop_back(); // Safe b.c. the timer uses a shared_ptr_task.
					continue;
				}

				auto const duration(next_timer.firing_time - clock_type::now());
				auto const duration_ms(chrono::duration_cast <chrono::milliseconds>(duration).count());
				if (duration_ms <= 0)
				{
					// FIXME: Verify that we could fire while not holding m_timer_mutex?
					// FIXME: The concurrent queue could benefit from sorting the timers by queue and then bulk inserting the operations.
					next_timer.timer->fire();

					// If the timer repeats, re-schedule.
					if (next_timer.timer->repeats())
					{
						next_timer.firing_time += next_timer.timer->interval();
						std::make_heap(m_timer_entries.begin(), m_timer_entries.end(), heap_cmp_type{});
					}
					else
					{
						// m_timers may be empty after popping.
						std::pop_heap(m_timer_entries.begin(), m_timer_entries.end(), heap_cmp_type{});
						m_timer_entries.pop_back();
					}
				}
				else
				{
					// m_timer_entries not empty.
					next_firing_time = m_timer_entries.front().firing_time - clock_type::now();
					break;
				}
			}
			while (!m_timer_entries.empty());
		}

		return next_firing_time;
	}


	auto manager_base::schedule_timer(
		timer::duration_type const interval,
		bool repeats,
		queue &qq,
		timer::task_type tt
	) -> timer_ptr										// Thread-safe.
	{
		auto retval([&]{
			std::lock_guard lock(m_timer_mutex);
			auto const now(clock_type::now());

			// Insert to the correct position.
			auto &entry(m_timer_entries.emplace_back(now + interval, std::make_shared <timer>(qq, std::move(tt), interval, repeats)));
			auto retval(entry.timer);
			std::push_heap(m_timer_entries.begin(), m_timer_entries.end(), heap_cmp_type{});
			return retval;
		}());

		trigger_event(event_type::wake_up);
		return retval;
	}


	void manager_base::start_thread_and_run(std::jthread &thread)
	{
		auto const res(m_is_running_worker.exchange(true, std::memory_order_acq_rel));
		libbio_assert(!res);
		thread = std::jthread([this]{
			block_signals();
			run();
		});
	}


	void install_sigchld_handler(manager &mgr, queue &qq, sigchld_handler &handler)
	{
		mgr.add_signal_event_source(SIGCHLD, qq, [&handler](signal_source const &source){
			bool did_report_error(false);
			while (true)
			{
				int status(0);
				auto const pid(::waitpid(-1, &status, WNOHANG | WUNTRACED));
				if (pid <= 0)
					break;

				if (WIFEXITED(status))
				{
					auto const exit_status(WEXITSTATUS(status));
					if (0 != exit_status)
					{
						did_report_error = true;
						char const *reason{nullptr};
						switch (exit_status)
						{
							case 127:
								reason = "command not found";
								break;

							case 126:
								reason = "command invoked cannot execute";
								break;

							case 69: // EX_UNAVAILABLE
								reason = "service unavailable";
								break;

							case 71: // EX_OSERR
								reason = "unknown error from execvp()";
								break;

							case 74: // EX_IOERR
								reason = "an I/O error occurred";
								break;

							default:
								break;
						}

						handler.child_did_exit_with_nonzero_status(pid, exit_status, reason);
					}
				}
				else if (WIFSIGNALED(status))
				{
					did_report_error = true;
					auto const signal_number(WTERMSIG(status));
					handler.child_received_signal(pid, signal_number);
				}
			}

			handler.finish_handling(did_report_error);
		});
	}
}

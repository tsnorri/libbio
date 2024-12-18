/*
 * Copyright (c) 2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <catch2/catch_test_macros.hpp>
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <libbio/dispatch.hh>
#include <libbio/dispatch/event.hh>
#include <signal.h>						// ::kill
#include <sys/types.h>
#include <sys/signal.h>
#include <unistd.h>						// ::getpid
#include "atomic_variable.hh"

namespace dispatch	= libbio::dispatch;
namespace events	= libbio::dispatch::events;
namespace tests		= libbio::tests;


namespace {

	struct pipe_handle
	{
		int fds[2]{-1, -1};

		~pipe_handle()
		{
			::close(fds[0]);
			::close(fds[1]);
		}

		void prepare()
		{
			if (-1 == ::pipe(fds))
				FAIL(::strerror(errno));
		}
	};


	void signal_handler(int sig, siginfo_t * info, void *)
	{
		sigset_t mask{};
		if (-1 == sigemptyset(&mask))
			std::abort();

		if (-1 == ::pthread_sigmask(SIG_BLOCK, nullptr, &mask))
			std::abort();

		bool const is_member(sigismember(&mask, sig));
		std::abort();
	}
}


SCENARIO("dispatch::events::manager can detect that a file descriptor is ready for writing", "[dispatch_event_manager]")
{
	GIVEN("a pipe")
	{
		pipe_handle pipe;
		pipe.prepare();

		WHEN("dispatch::events::manager monitors the write end")
		{
			tests::atomic_bool status{};
			dispatch::thread_pool thread_pool;
			dispatch::parallel_queue queue(thread_pool);
			std::jthread manager_thread;
			events::manager event_manager;

			event_manager.setup();
			event_manager.start_thread_and_run(manager_thread);
			event_manager.add_file_descriptor_write_event_source(pipe.fds[1], queue, [&](events::file_descriptor_source &){
				status.assign(true);
			});

			THEN("an event is received")
			{
				auto const lock(status.wait_and_lock());
				CHECK(true == status.value());
			}
		}
	}
}


SCENARIO("dispatch::events::manager can detect that a file descriptor is ready for reading", "[dispatch_event_manager]")
{
	GIVEN("a pipe")
	{
		pipe_handle pipe;
		pipe.prepare();

		WHEN("dispatch::events::manager monitors the read end")
		{
			tests::atomic_bool status{};
			dispatch::thread_pool thread_pool;
			dispatch::parallel_queue queue(thread_pool);
			std::jthread manager_thread;
			events::manager event_manager;

			event_manager.setup();
			event_manager.start_thread_and_run(manager_thread);
			event_manager.add_file_descriptor_read_event_source(pipe.fds[0], queue, [&](events::file_descriptor_source &){
				status.assign(true);
			});

			AND_WHEN("a byte is written to the write end of the pipe")
			{
				{
					char const val{1};
					if (-1 == ::write(pipe.fds[1], &val, 1))
						FAIL(::strerror(errno));
				}

				THEN("an event is received")
				{
					auto const lock(status.wait_and_lock());
					CHECK(true == status.value());
				}
			}
		}
	}
}


SCENARIO("dispatch::events::manager can detect that a signal has been received", "[dispatch_event_manager]")
{
	// For debugging; should not be needed since the signal is blocked anyway.
	{
		struct sigaction act{};
		act.sa_sigaction = &signal_handler;
		act.sa_flags = SA_SIGINFO;
		if (-1 == sigemptyset(&act.sa_mask))
			std::abort();

		if (-1 == ::sigaction(SIGQUIT, &act, nullptr))
			std::abort();
	}

	WHEN("A signal is blocked")
	{
		events::signal_mask mask;
		mask.add(SIGQUIT);

		AND_WHEN("dispatch::events::manager monitors the signal")
		{
			tests::atomic_bool status{};
			dispatch::thread_pool thread_pool;
			dispatch::parallel_queue queue(thread_pool);
			std::jthread manager_thread;
			events::manager event_manager;

			event_manager.setup();
			event_manager.start_thread_and_run(manager_thread);
			event_manager.add_signal_event_source(SIGQUIT, queue, [&](events::signal_source &){
				status.assign(true);
			});

			AND_WHEN("the signal is received")
			{
				REQUIRE(0 == ::kill(::getpid(), SIGQUIT));

				THEN("an event is received")
				{
					auto const lock(status.wait_and_lock());
					CHECK(true == status.value());
				}
			}
		}
	}
}


SCENARIO("dispatch::events::manager can report non-repeating timer events", "[dispatch_event_manager]")
{
	WHEN("dispatch::events::manager monitors a timer")
	{
		tests::atomic_uint32_t counter{};
		dispatch::thread_pool thread_pool;
		dispatch::parallel_queue queue(thread_pool);
		std::jthread manager_thread;
		events::manager event_manager;

		event_manager.setup();
		event_manager.start_thread_and_run(manager_thread);
		event_manager.schedule_timer(std::chrono::milliseconds(100), false, queue, [&](events::timer &){
			auto const lock(counter.lock());
			++counter.value();
		});

		THEN("an event is received exactly once")
		{
			auto const lock(counter.wait_and_lock());
			CHECK(1 == counter.value());
		}
	}
}


SCENARIO("dispatch::events::manager can report repeating timer events", "[dispatch_event_manager]")
{
	WHEN("dispatch::events::manager monitors a timer")
	{
		tests::atomic_uint32_t counter{};
		dispatch::thread_pool thread_pool;
		dispatch::parallel_queue queue(thread_pool);
		std::jthread manager_thread;
		events::manager event_manager;

		event_manager.setup();
		event_manager.start_thread_and_run(manager_thread);
		auto const interval(std::chrono::milliseconds(100));
		INFO("Firing interval: " << interval);
		event_manager.schedule_timer(interval, true, queue, [&](events::timer &){
			auto const lock(counter.lock());
			++counter.value();
		});

		THEN("an event is received at least twice")
		{
			auto const lock(counter.wait_and_lock());
			INFO("counter.value(): " << counter.value());
			CHECK(2 <= counter.value());
		}
	}
}


SCENARIO("dispatch::events::manager can report repeating timer events for multiple timers", "[dispatch_event_manager]")
{
	WHEN("dispatch::events::manager monitors a pair of timers")
	{
		tests::atomic_uint32_t c1{}, c2{};
		dispatch::thread_pool thread_pool;
		dispatch::parallel_queue queue(thread_pool);
		std::jthread manager_thread;
		events::manager event_manager;

		event_manager.setup();
		event_manager.start_thread_and_run(manager_thread);
		auto const i1(std::chrono::milliseconds(200));
		auto const i2(std::chrono::milliseconds(150));
		INFO("Firing intervals: " << i1 << ", " << i2);

		event_manager.schedule_timer(i1, true, queue, [&](events::timer &){
			auto const lock(c1.lock());
			++c1.value();
		});

		event_manager.schedule_timer(i2, true, queue, [&](events::timer &){
			auto const lock(c2.lock());
			++c2.value();
		});


		THEN("multiple events are received")
		{
			auto const l1(c1.wait_and_lock());
			auto const l2(c2.wait_and_lock());
			INFO("c1.value(): " << c1.value());
			INFO("c2.value(): " << c2.value());
			CHECK(2 <= c1.value());
			CHECK(2 <= c2.value());
			CHECK(c1.value() <= c2.value());
		}
	}
}

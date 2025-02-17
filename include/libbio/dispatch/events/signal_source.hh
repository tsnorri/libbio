/*
 * Copyright (c) 2023-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_DISPATCH_EVENTS_SIGNAL_SOURCE_HH
#define LIBBIO_DISPATCH_EVENTS_SIGNAL_SOURCE_HH

#include <libbio/dispatch/fwd.hh>
#include <libbio/dispatch/events/source.hh>
#include <libbio/dispatch/task_decl.hh>
#include <sys/types.h>
#include <utility>							// std::forward


namespace libbio::dispatch::events {

	class signal_source final : public source_tpl <signal_source>
	{
	public:
		typedef task_t <signal_source &>	task_type;

	private:
		signal_type	m_signal{-1};

	public:
		signal_source(
			queue &qq,
			task_type &&tt,
			signal_type const sig
		):
			source_tpl(qq, std::move(tt)),
			m_signal(sig)
		{
		}

		signal_type signal() const { return m_signal; }
	};

	typedef signal_source::task_type signal_task;


	struct sigchld_handler
	{
		virtual ~sigchld_handler() {}
		virtual void child_did_exit_with_nonzero_status(pid_t const pid, int const exit_status, char const *reason) = 0;
		virtual void child_received_signal(pid_t const pid, int const signal_number) = 0;
		virtual void finish_handling(bool const did_report_error) = 0;
	};
}


namespace libbio::dispatch::detail {

	template <>
	struct member_callable_target <events::signal_source>
	{
		typedef events::source_tpl <events::signal_source>	type;
	};
}

#endif

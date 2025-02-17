/*
 * Copyright (c) 2023-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_DISPATCH_EVENTS_TIMER_HH
#define LIBBIO_DISPATCH_EVENTS_TIMER_HH

#include <chrono>
#include <libbio/dispatch/fwd.hh>
#include <libbio/dispatch/events/source.hh>
#include <libbio/dispatch/task_decl.hh>
#include <limits>


namespace libbio::dispatch::events {

	class timer final : public source_tpl <timer>
	{
	public:
		typedef task_t <timer &>			task_type;
		typedef std::chrono::steady_clock	clock_type;
		typedef clock_type::duration		duration_type;

		constexpr static inline duration_type const DURATION_MAX{std::numeric_limits <duration_type>::max()};

	private:
		duration_type		m_interval{};
		bool				m_repeats{};

	public:
		template <typename t_task>
		timer(queue &qq, t_task &&tt, duration_type const interval, bool repeats):
			source_tpl(qq, std::forward <t_task>(tt)),
			m_interval(interval),
			m_repeats(repeats)
		{
		}

		duration_type interval() const { return m_interval; }
		bool repeats() const { return m_repeats; }
	};

	typedef timer::task_type timer_task;
}


namespace libbio::dispatch::detail {

	template <>
	struct member_callable_target <events::timer>
	{
		typedef events::source_tpl <events::timer>	type;
	};
}

#endif

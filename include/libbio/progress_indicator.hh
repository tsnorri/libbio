/*
 * Copyright (c) 2017-2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_PROGRESS_INDICATOR_HH
#define LIBBIO_PROGRESS_INDICATOR_HH

#if !(defined(LIBBIO_NO_PROGRESS_INDICATOR) && LIBBIO_NO_PROGRESS_INDICATOR)

#include <atomic>
#include <boost/format.hpp>
#include <chrono>
#include <cstdio>
#include <mutex>
#include <libbio/dispatch.hh>
#include <libbio/utility.hh>
#include <unistd.h>


namespace libbio {
	
	struct progress_indicator_delegate
	{
		virtual ~progress_indicator_delegate() {}
		virtual std::size_t progress_step_max() const = 0;
		virtual std::size_t progress_current_step() const = 0;
		virtual void progress_log_extra() const = 0;
	};
	
	
	// Display a counter or a progress bar (for now), use libdispatch’s run loop to update.
	class progress_indicator
	{
	public:
		typedef std::chrono::time_point <std::chrono::steady_clock> time_point_type;
		
	protected:
		enum indicator_type : uint8_t
		{
			NONE = 0,
			COUNTER,
			PROGRESS_BAR
		};

	protected:
		dispatch_ptr <dispatch_source_t>	m_message_timer{};
		dispatch_ptr <dispatch_source_t>	m_signal_source{};
		
		std::mutex							m_message_mutex;
		std::string							m_message;
		
		time_point_type						m_start_time{};
		std::size_t							m_window_width{};
		std::size_t							m_message_length{};
		std::size_t							m_current_max{};
		std::atomic <indicator_type>		m_indicator_type{indicator_type::NONE};
		std::atomic_bool					m_timer_active{};
		bool								m_is_installed{};
		
		progress_indicator_delegate			*m_delegate{};
		
	public:
		bool is_stderr_interactive() const { return isatty(fileno(stderr)); }
		bool is_installed() const { return m_is_installed; }
		
		progress_indicator() = default;
		
		// Call from the main queue.
		void install();
		void uninstall();
		
		void log_with_progress_bar(std::string const &message, progress_indicator_delegate &delegate) { if (m_is_installed) setup_and_start(message, delegate, indicator_type::PROGRESS_BAR); }
		void log_with_counter(std::string const &message, progress_indicator_delegate &delegate) { if (m_is_installed) setup_and_start(message, delegate, indicator_type::COUNTER); }
		void end_logging();
		void end_logging_mt();
		
	protected:
		void setup_and_start(std::string const &message, progress_indicator_delegate &delegate, indicator_type const indicator);
		
		// Call from the main queue.
		void resume_timer_mt();
		void handle_window_size_change_mt();
		void update_mt();
	};
	
}

#endif
#endif

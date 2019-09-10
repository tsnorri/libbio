/*
 * Copyright (c) 2017-2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#if !(defined(LIBBIO_NO_PROGRESS_INDICATOR) && LIBBIO_NO_PROGRESS_INDICATOR)

#include <algorithm>
#include <boost/format.hpp>
#include <cmath>
#include <iomanip>
#include <libbio/progress_bar.hh>
#include <libbio/progress_indicator.hh>
#include <sys/ioctl.h>

namespace ch = std::chrono;


namespace libbio {
	
	void progress_indicator::install()
	{
		m_is_installed = true;
		dispatch_queue_t main_queue(dispatch_get_main_queue());
		
		{
			// Update timer.
			dispatch_ptr <dispatch_source_t> message_timer(
				dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, main_queue),
				false
			);
			
				// Window size change signal event source.
			dispatch_ptr <dispatch_source_t> signal_source(
				dispatch_source_create(DISPATCH_SOURCE_TYPE_SIGNAL, SIGWINCH, 0, main_queue),
				false
			);
		
			m_message_timer = std::move(message_timer);
			m_signal_source = std::move(signal_source);
		}
		
		dispatch_source_set_timer(*m_message_timer, dispatch_time(DISPATCH_TIME_NOW, 0), 100 * 1000 * 1000, 50 * 1000 * 1000);
		dispatch(this).source_set_event_handler <&progress_indicator::handle_window_size_change_mt>(*m_signal_source);
		
		// Set the initial window size.
		dispatch(this).async <&progress_indicator::handle_window_size_change_mt>(main_queue);
		
		dispatch_resume(*m_signal_source);
	}
	
	
	void progress_indicator::uninstall()
	{
		if (m_is_installed)
		{
			// Deallocating a suspended source is considered a bug.
			dispatch_resume(*m_message_timer);
		
			dispatch_source_cancel(*m_message_timer);
			dispatch_source_cancel(*m_signal_source);

			m_is_installed = false;
		}
	}
	
	
	void progress_indicator::setup_and_start(std::string const &message, progress_indicator_delegate &delegate, indicator_type const indicator)
	{
		// FIXME: since end_logging is synchronous, the caller needs to wait for its completion before starting the new logging task. Therefore the mutex below should be unnecessary.
		auto const message_len(strlen_utf8(message));
		std::lock_guard <std::mutex> guard(m_message_mutex);
		m_delegate = &delegate;
		m_message = message;
		m_message_length = message_len;
		m_indicator_type = indicator;
		m_start_time = ch::steady_clock::now();
		m_current_max = m_delegate->progress_step_max();
		
		dispatch(this).async <&progress_indicator::resume_timer_mt>(dispatch_get_main_queue());
	}
	
	
	void progress_indicator::end_logging()
	{
		if (m_is_installed)
			dispatch(this).sync <&progress_indicator::end_logging_mt>(dispatch_get_main_queue());
	}

	
	void progress_indicator::end_logging_no_update()
	{
		if (m_is_installed)
			dispatch(this).sync <&progress_indicator::end_logging_no_update_mt>(dispatch_get_main_queue());
	}
	
	
	void progress_indicator::resume_timer_mt()
	{
		// Hide the cursor.
		std::cerr << "\33[?25l";

		m_timer_active = true;
		dispatch(this).source_set_event_handler <&progress_indicator::update_mt>(*m_message_timer);
		dispatch_resume(*m_message_timer);
	}
	
	
	void progress_indicator::handle_window_size_change_mt()
	{
		struct winsize ws;
		ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
		m_window_width = ws.ws_col;
		if (!m_window_width)
			m_window_width = 80;
		update_mt();
	}
	
	
	void progress_indicator::end_logging_mt(bool const should_update)
	{
		if (!m_is_installed)
			return;
		
		if (should_update)
			update_mt();
		dispatch_suspend(*m_message_timer);
		m_indicator_type = indicator_type::NONE;
		m_timer_active = false;
		m_delegate = nullptr;
		
		// Show the cursor.
		std::cerr << "\33[?25h" << std::endl;
	}
	
	
	void progress_indicator::update_mt()
	{
		if (!m_timer_active)
			return;
		
		switch (m_indicator_type)
		{
			case indicator_type::COUNTER:
			{
				std::lock_guard <std::mutex> guard(m_message_mutex);
				
				auto const current_step(m_delegate->progress_current_step());
				auto fmt(
					boost::format("%s % d ")
					% m_message
					% boost::io::group(std::setw(m_window_width - m_message_length - 2), current_step)
				);
				//std::cerr << '\r' << fmt << std::flush;
				std::cerr << "\33[1G" << fmt << std::flush;
				break;
			}
			
			case indicator_type::PROGRESS_BAR:
			{
				std::lock_guard <std::mutex> guard(m_message_mutex);
				
				float const step_max(m_current_max);
				float const current_step(m_delegate->progress_current_step());
				float const progress_val(current_step / step_max);
				auto const half(m_window_width / 2);
				//auto const pad(half < m_message_length ? 1 : half - m_message_length);
				auto const max_width(std::min(40UL, m_window_width - half - 1));
				
				progress_bar(std::cerr, progress_val, max_width, 0, m_message, m_start_time);
				
				m_delegate->progress_log_extra();
			}
			
			case indicator_type::NONE:
			default:
				break;
		}
	}
}

#endif

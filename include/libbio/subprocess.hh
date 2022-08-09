/*
 * Copyright (c) 2022 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_SUBPROCESS_HH
#define LIBBIO_SUBPROCESS_HH

#include <libbio/file_handle.hh>
#include <limits>
#include <tuple>


namespace libbio { namespace detail {
	
	std::tuple <pid_t, int>
	open_subprocess_for_writing(char * const args[]); // nullptr-terminated list of arguments.
}}


namespace libbio {
	
	class process_handle
	{
		static_assert(std::numeric_limits <pid_t>::min() <= -1); // We use -1 as a magic value.
		
	public:
		enum class close_status
		{
			unknown,
			exit_called,
			terminated_by_signal,
			stopped_by_signal
		};
		
		typedef std::tuple <close_status, int, pid_t>	close_return_t;
		
	protected:
		pid_t				m_pid{-1};
		
	public:
		process_handle() = default;
		process_handle(process_handle const &) = delete;
		process_handle &operator=(process_handle const &) = delete;
		
		explicit process_handle(pid_t const pid):
			m_pid(pid)
		{
		}
		
		process_handle(process_handle &&other):
			m_pid(other.m_pid)
		{
			other.m_pid = -1;
		}
		
		inline process_handle &operator=(process_handle &&other) &;
		pid_t pid() const { return m_pid; }
		close_return_t close();
		~process_handle() { if (-1 != m_pid) close(); }
	};
	
	
	class subprocess
	{
	public:
		typedef process_handle::close_return_t	close_return_t;
		
	protected:
		process_handle	m_process{};		// Data member destructors called in reverse order; important for closing the pipe first.
		file_handle		m_write_handle{};
		
	protected:
		explicit subprocess(std::tuple <pid_t, int> const &pid_fd_tuple):
			m_process(std::get <0>(pid_fd_tuple)),
			m_write_handle(std::get <1>(pid_fd_tuple))
		{
		}
	
	public:
		subprocess() = default;
		
		explicit subprocess(char * const args[]):
			subprocess(detail::open_subprocess_for_writing(args))
		{
		}
		
		inline close_return_t close();
		file_handle &write_handle() { return m_write_handle; }
		file_handle const &write_handle() const { return m_write_handle; }
	};
	
	
	process_handle &process_handle::operator=(process_handle &&other) &
	{
		if (this != &other)
		{
			using std::swap;
			swap(m_pid, other.m_pid);
		}
		
		return *this;
	}
	
	
	auto subprocess::close() -> close_return_t
	{
		m_write_handle.close();
		return m_process.close();
	}
}

#endif

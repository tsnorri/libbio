/*
 * Copyright (c) 2022 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <cstring>
#include <fcntl.h>
#include <libbio/subprocess.hh>
#include <stdexcept>
#include <sys/wait.h>
#include <unistd.h>


namespace libbio { namespace detail {
	
	static const std::tuple <std::size_t, int, int>
	s_values_for_handle_spec[]{
		{0, STDIN_FILENO, O_RDONLY},
		{1, STDOUT_FILENO, O_WRONLY},
		{1, STDERR_FILENO, O_WRONLY}
	};
	
	
	inline std::tuple <std::size_t, int, int> values_for_handle_spec(subprocess_handle_spec const handle_spec)
	{
		auto const idx1(to_underlying(handle_spec));
		libbio_assert_lt(0, idx1);
		auto const idx(idx1 - 1);
		libbio_assert_lt(idx, sizeof(s_values_for_handle_spec) / sizeof(s_values_for_handle_spec[0]));
		return s_values_for_handle_spec[idx];
	}
	
	
	std::tuple <pid_t, int, int, int>
	open_subprocess(char * const args[], subprocess_handle_spec const handle_spec) // nullptr-terminated list of arguments.
	{
		// Note that if this function is invoked simultaneously from different threads,
		// the subprocesses can leak file descriptors b.c. the ones resulting from the
		// call to pipe() may not have been closed or marked closed-on-exec.
		
		// Create the requested pipes.
		int fds[3][2]{{-1, -1}, {-1, -1}, {-1, -1}};
		
		auto process_all_fds([&fds](auto &&cb){
			for (std::size_t i(0); i < 3; ++i)
				cb(static_cast <subprocess_handle_spec>(1 + i), fds[i]);
		});
		
		auto process_open_fds([&process_all_fds, handle_spec](auto &&cb){
			process_all_fds([&cb, handle_spec](auto const curr_spec, auto &fd_pair){
				if (to_underlying(curr_spec & handle_spec))
					cb(curr_spec, fd_pair);
			});
		});
		
		process_open_fds([](auto const, auto &fd_pair){
			if (0 != ::pipe(fd_pair))
				throw std::runtime_error(::strerror(errno));
		});
		
		auto const pid(::fork());
		switch (pid)
		{
			case -1:
			{
				auto const * const err(::strerror(errno));
				process_open_fds([](auto const, auto &fd_pair){
					::close(fd_pair[0]);
					::close(fd_pair[1]);
				});
				throw std::runtime_error(err);
				break;
			}
			
			case 0: // Child.
			{
				// Try to make the child continue when debugging.
				::signal(SIGTRAP, SIG_IGN);
				
				process_all_fds([handle_spec](auto const curr_spec, auto &fd_pair){
					auto const [sp_fd_idx, fno, open_flags] = detail::values_for_handle_spec(curr_spec);
					
					if (to_underlying(curr_spec & handle_spec))
					{
						if (-1 != ::dup2(fd_pair[sp_fd_idx], fno))			::_exit(69);	// EX_UNAVAILABLE in sysexits.h.
						if (-1 != ::close(fd_pair[0]))						::_exit(69);
						if (-1 != ::close(fd_pair[1]))						::_exit(69);
					}
					else
					{
						if (-1 != ::close(fno))								::_exit(69);
						if (-1 != ::openat(fno, "/dev/null", open_flags))	::_exit(69);
					}
				});
				
				::execvp(*args, args);
				
				switch (errno)
				{
					case E2BIG:
					case EACCES:
					case ENAMETOOLONG:
					case ENOENT:
					case ELOOP:
					case ENOTDIR:
						::_exit(127);
					
					case EFAULT:
					case ENOEXEC:
					case ENOMEM:
					case ETXTBSY:
						::_exit(126);
					
					case EIO:
						::_exit(74);	// EX_IOERR in sysexits.h.
					
					default:
						::_exit(71);	// EX_OSERR in sysexits.h
				}
				
				break;
			}
			
			default: // Parent.
			{
				bool should_throw{};
				char const *err{};
				process_open_fds([&should_throw, &err](auto const curr_spec, auto &fd_pair){
					auto const [sp_fd_idx, fno, open_flags] = detail::values_for_handle_spec(curr_spec);
					auto const parent_fd_idx(1 - sp_fd_idx);
					
					::close(fd_pair[sp_fd_idx]);
					if (-1 == ::fcntl(fd_pair[parent_fd_idx], F_SETFD, FD_CLOEXEC))
					{
						if (!should_throw)
						{
							err = ::strerror(errno);
							should_throw = true;
						}
					}
				});
				
				if (should_throw)
				{
					// Clean up if there was an error.
					process_open_fds([](auto const curr_spec, auto &fd_pair){
						auto const [sp_fd_idx, fno, open_flags] = detail::values_for_handle_spec(curr_spec);
						auto const parent_fd_idx(1 - sp_fd_idx);
						::close(fd_pair[parent_fd_idx]);
					});
					
					throw std::runtime_error(err);
				}
				
				break;
			}
		}
		
		return {pid, fds[0][1], fds[1][0], fds[2][0]}; // Return stdin’s write end and the read end of the other two.
	}
}}


namespace libbio {
	
	auto process_handle::close() -> close_return_t
	{
		int status{};
		auto const res(::waitpid(m_pid, &status, WNOHANG | WUNTRACED));
		auto const pid(m_pid);
		m_pid = -1;
		
		if (-1 == res)
			throw std::runtime_error(::strerror(errno));
		
		if (WIFEXITED(status))
			return {close_status::exit_called, WEXITSTATUS(status), pid};
		else if (WIFSIGNALED(status))
			return {close_status::terminated_by_signal, WTERMSIG(status), pid};
		else if (WIFSTOPPED(status))
			return {close_status::stopped_by_signal, WSTOPSIG(status), pid};
		
		return {close_status::unknown, 0, pid};
	}
}

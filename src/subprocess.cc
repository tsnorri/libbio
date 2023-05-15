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

	struct subprocess_handle_trait
	{
		std::size_t	sp_fd_idx{};	// Index in the array passed to ::pipe()
		int			fd{};
		int			oflags{};		// Flags passed to ::open().
	};
	
	static std::array const
	s_handle_traits{
		subprocess_handle_trait{0, STDIN_FILENO, O_RDONLY},
		subprocess_handle_trait{1, STDOUT_FILENO, O_WRONLY},
		subprocess_handle_trait{1, STDERR_FILENO, O_WRONLY}
	};
	
	
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
			{
				auto const &trait(s_handle_traits[i]);
				cb(static_cast <subprocess_handle_spec>(0x1 << i), fds[i], trait);
			}
		});
		
		auto process_open_fds([&process_all_fds, handle_spec](auto &&cb){
			process_all_fds([&cb, handle_spec](auto const curr_spec, auto &fd_pair, auto const &trait){
				if (to_underlying(curr_spec & handle_spec))
					cb(curr_spec, fd_pair, trait);
			});
		});
		
		process_open_fds([](auto const, auto &fd_pair, auto const &trait){
			if (0 != ::pipe(fd_pair))
				throw std::runtime_error(::strerror(errno));
		});
		
		auto const pid(::fork());
		switch (pid)
		{
			case -1:
			{
				auto const * const err(::strerror(errno));
				process_open_fds([](auto const, auto &fd_pair, auto const &trait){
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
				
				process_all_fds([handle_spec](auto const curr_spec, auto &fd_pair, auto const &trait){
					if (to_underlying(curr_spec & handle_spec))
					{
						// Keep opened.
						if (-1 == ::dup2(fd_pair[trait.sp_fd_idx], trait.fd))		::_exit(69);	// EX_UNAVAILABLE in sysexits.h.
						if (-1 == ::close(fd_pair[0]))								::_exit(69);
						if (-1 == ::close(fd_pair[1]))								::_exit(69);
					}
					else
					{
						// Re-open with /dev/null.
						if (-1 == ::close(trait.fd))								::_exit(69);
						if (-1 == ::openat(trait.fd, "/dev/null", trait.oflags))	::_exit(69);
					}
				});
				
				::execvp(*args, args); // Returns only on error (with the value -1).
				
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
				process_open_fds([&should_throw, &err](auto const curr_spec, auto &fd_pair, auto const &trait){
					auto const parent_fd_idx(1 - trait.sp_fd_idx);
					
					::close(fd_pair[trait.sp_fd_idx]);
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
					process_open_fds([](auto const curr_spec, auto &fd_pair, auto const &trait){
						auto const parent_fd_idx(1 - trait.sp_fd_idx);
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

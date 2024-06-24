/*
 * Copyright (c) 2022-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <cstring>
#include <fcntl.h>
#include <libbio/subprocess.hh>
#include <mutex>
#include <signal.h>
#include <stdexcept>
#include <sys/wait.h>
#include <unistd.h>

namespace lb	= libbio;


#define PARENT_ERROR(EXECUTION_STATUS)					open_subprocess_error(subprocess_status((EXECUTION_STATUS), __LINE__, errno))
#define RETURN_FAILURE(EXECUTION_STATUS)				do { retval.status = subprocess_status((EXECUTION_STATUS), __LINE__, errno); return retval; } while (false)
#define EXIT_SUBPROCESS(EXECUTION_STATUS, EXIT_STATUS)	do { do_exit(status_pipe[1], (EXIT_STATUS), (EXECUTION_STATUS), __LINE__, errno); } while (false)


namespace {
	
	void do_exit(
		int const fd,
		int const exit_status,
		lb::execution_status_type const execution_status,
		std::uint32_t const line,
		int const error
	)
	{
		lb::subprocess_status const st{execution_status, line, error};
		if (sizeof(st) != ::write(fd, &st, sizeof(st)))
			std::abort();
		
		::close(fd);
		::_exit(exit_status);
	}
	
	
	struct open_subprocess_error
	{
		lb::subprocess_status status;
	};
}


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
	
	
	open_subprocess_result
	open_subprocess(char const * const args[], subprocess_handle_spec const handle_spec) // nullptr-terminated list of arguments.
	{
		// Note that if this function is invoked simultaneously from different threads,
		// the subprocesses can leak file descriptors b.c. the ones resulting from the
		// call to pipe() may not have been closed or marked closed-on-exec.
		
		
		open_subprocess_result retval;
		int status_pipe[2]{-1, -1};						// For detecting execvp().
		int io_fds[3][2]{{-1, -1}, {-1, -1}, {-1, -1}};	// The requested pipes.
		
		auto process_all_fds([&io_fds](auto &&cb){
			for (std::size_t i(0); i < 3; ++i)
			{
				auto const &trait(s_handle_traits[i]);
				cb(static_cast <subprocess_handle_spec>(0x1 << i), io_fds[i], trait);
			}
		});
		
		auto process_open_fds([&process_all_fds, handle_spec](auto &&cb){
			process_all_fds([&cb, handle_spec](auto const curr_spec, auto &fd_pair, auto const &trait){
				if (to_underlying(curr_spec & handle_spec))
					cb(curr_spec, fd_pair, trait);
			});
		});
		
		// Status pipe.
		if (0 != ::pipe(status_pipe))
			RETURN_FAILURE(execution_status_type::file_descriptor_handling_failed);
		
		// Create the requested pipes.
		try
		{
			process_open_fds([](auto const, auto &fd_pair, auto const &trait){
				if (0 != ::pipe(fd_pair))
					throw PARENT_ERROR(execution_status_type::file_descriptor_handling_failed);
			});
		}
		catch (open_subprocess_error const &err)
		{
			retval.status = err.status;
			return retval;
		}

		auto const pid(::fork());
		switch (pid)
		{
			case -1:
			{
				process_open_fds([](auto const, auto &fd_pair, auto const &trait){
					::close(fd_pair[0]);
					::close(fd_pair[1]);
				});
				RETURN_FAILURE(execution_status_type::fork_failed);
			}
			
			case 0: // Child.
			{
				// Close the read end of the status pipe.
				::close(status_pipe[0]);
				
				// Mark close-on-exec.
				if (-1 == ::fcntl(status_pipe[1], F_SETFD, FD_CLOEXEC))
					std::abort(); // Unable to report this error.
				
				// Try to make the child continue when debugging.
				::signal(SIGTRAP, SIG_IGN);
				
				process_all_fds([handle_spec, &status_pipe](auto const curr_spec, auto &fd_pair, auto const &trait){
					if (to_underlying(curr_spec & handle_spec))
					{
						// Keep opened.
						if (-1 == ::dup2(fd_pair[trait.sp_fd_idx], trait.fd))		EXIT_SUBPROCESS(execution_status_type::file_descriptor_handling_failed, 69);	// EX_UNAVAILABLE in sysexits.h.
						if (-1 == ::close(fd_pair[0]))								EXIT_SUBPROCESS(execution_status_type::file_descriptor_handling_failed, 69);
						if (-1 == ::close(fd_pair[1]))								EXIT_SUBPROCESS(execution_status_type::file_descriptor_handling_failed, 69);
					}
					else
					{
						// Re-open with /dev/null.
						if (-1 == ::close(trait.fd))								EXIT_SUBPROCESS(execution_status_type::file_descriptor_handling_failed, 69);
						if (-1 == ::openat(trait.fd, "/dev/null", trait.oflags))	EXIT_SUBPROCESS(execution_status_type::file_descriptor_handling_failed, 69);
					}
				});
				
				// According to POSIX, “the argv[] and envp[] arrays of pointers and the strings to which those arrays point
				// shall not be modified by a call to one of the exec functions, except as a consequence of replacing the
				// process image.” (https://pubs.opengroup.org/onlinepubs/009604499/functions/exec.html)
				// Hence the const_cast below should be safe.
				::execvp(*args, const_cast <char * const *>(args)); // Returns only on error (with the value -1).
				
				// execvp only returns if an error occurred.
				switch (errno)
				{
					case E2BIG:
					case EACCES:
					case ENAMETOOLONG:
					case ENOENT:
					case ELOOP:
					case ENOTDIR:
						EXIT_SUBPROCESS(execution_status_type::exec_failed, 127);
					
					case EFAULT:
					case ENOEXEC:
					case ENOMEM:
					case ETXTBSY:
						EXIT_SUBPROCESS(execution_status_type::exec_failed, 126);
					
					case EIO:
						EXIT_SUBPROCESS(execution_status_type::exec_failed, 74);	// EX_IOERR in sysexits.h.
					
					default:
						EXIT_SUBPROCESS(execution_status_type::exec_failed, 71);	// EX_OSERR in sysexits.h
				}
				
				break;
			}
			
			default: // Parent.
			{
				// Close the write end of the status pipe.
				::close(status_pipe[1]);
				
				try
				{
					process_open_fds([](auto const curr_spec, auto &fd_pair, auto const &trait){
						auto const parent_fd_idx(1 - trait.sp_fd_idx);
						
						::close(fd_pair[trait.sp_fd_idx]);
						if (-1 == ::fcntl(fd_pair[parent_fd_idx], F_SETFD, FD_CLOEXEC))
							throw PARENT_ERROR(execution_status_type::file_descriptor_handling_failed);
					});
					
					{
						auto const read_amt(::read(status_pipe[0], &retval.status, sizeof(retval.status)));
						switch (read_amt)
						{
							case -1:
								::close(status_pipe[0]);
								throw PARENT_ERROR(execution_status_type::file_descriptor_handling_failed);
							
							case 0:
							case (sizeof(retval.status)):
								break;
							
							default:
								::close(status_pipe[0]);
								errno = EBADMSG;
								throw PARENT_ERROR(execution_status_type::file_descriptor_handling_failed);
						}
					}
				}
				catch (open_subprocess_error const &err)
				{
					// Clean up if there was an error.
					process_open_fds([](auto const curr_spec, auto &fd_pair, auto const &trait){
						auto const parent_fd_idx(1 - trait.sp_fd_idx);
						::close(fd_pair[parent_fd_idx]);
					});
					
					::kill(pid, SIGTERM);
					
					retval.status = err.status;
					return retval;
				}
			}
		}
		
		retval.pid = pid;
		// Return stdin’s write end and the read end of the other two.
		retval.handles[0] = io_fds[0][1];
		retval.handles[1] = io_fds[1][0];
		retval.handles[2] = io_fds[2][0];
		return retval;
	}
}}


namespace libbio {
	
	auto process_handle::close() -> close_return_type
	{
		while (true)
		{
			int status{};
			auto const res(::waitpid(m_pid, &status, WUNTRACED));
			auto const pid(m_pid);
			m_pid = -1;
			
			if (-1 == res)
			{
				switch (errno)
				{
					case EINTR:
						continue; // Continues the while loop.
					
					case ECHILD:
						return {close_status::exit_called, 0, pid}; // FIXME: we’re being optimistic w.r.t. the exit status.
				}
				
				throw std::runtime_error(std::strerror(errno));
			}
			
			if (WIFEXITED(status))
				return {close_status::exit_called, WEXITSTATUS(status), pid};
			else if (WIFSIGNALED(status))
				return {close_status::terminated_by_signal, WTERMSIG(status), pid};
			else if (WIFSTOPPED(status))
				return {close_status::stopped_by_signal, WSTOPSIG(status), pid};
			
			return {close_status::unknown, 0, pid};
		}
	}
	
	
	void subprocess_status::output_status(std::ostream &os, bool const detailed)
	{
		switch (execution_status)
		{
			case execution_status_type::no_error:
				return;
				
			case execution_status_type::file_descriptor_handling_failed:
				os << "File descriptor handling failed";
				break;
			case execution_status_type::fork_failed:
				os << "Fork failed";
				break;
			case execution_status_type::exec_failed:
				os << "Execution failed";
				break;
		}

		os << "; " << std::strerror(error);

		if (detailed)
			os << "(line " << line << ")";
	}
}

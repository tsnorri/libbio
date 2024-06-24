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
		lb::detail::subprocess_status const st{execution_status, line, error};
		if (sizeof(st) != ::write(fd, &st, sizeof(st)))
			std::abort();
		
		::close(fd);
		::_exit(exit_status);
	}
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
	
	
	std::tuple <pid_t, int, int, int, subprocess_status>
	open_subprocess(char const * const args[], subprocess_handle_spec const handle_spec) // nullptr-terminated list of arguments.
	{
		// Note that if this function is invoked simultaneously from different threads,
		// the subprocesses can leak file descriptors b.c. the ones resulting from the
		// call to pipe() may not have been closed or marked closed-on-exec.
		
		// Status object for communicating errors to the parent process.
		subprocess_status status;
		
		// For detecting execvp().
		int status_pipe[2]{-1, -1};
		if (0 != ::pipe(status_pipe))
			throw std::runtime_error(std::strerror(errno));
		
		// Create the requested pipes.
		int io_fds[3][2]{{-1, -1}, {-1, -1}, {-1, -1}};
		
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
		
		process_open_fds([](auto const, auto &fd_pair, auto const &trait){
			if (0 != ::pipe(fd_pair))
				throw std::runtime_error(std::strerror(errno));
		});

		auto const pid(::fork());
		switch (pid)
		{
			case -1:
			{
				auto const * const err(std::strerror(errno));
				process_open_fds([](auto const, auto &fd_pair, auto const &trait){
					::close(fd_pair[0]);
					::close(fd_pair[1]);
				});
				throw std::runtime_error(err);
				break;
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
				
				bool should_throw{};
				char const *err{};
				process_open_fds([&should_throw, &err](auto const curr_spec, auto &fd_pair, auto const &trait){
					auto const parent_fd_idx(1 - trait.sp_fd_idx);
					
					::close(fd_pair[trait.sp_fd_idx]);
					if (-1 == ::fcntl(fd_pair[parent_fd_idx], F_SETFD, FD_CLOEXEC))
					{
						if (!should_throw)
						{
							err = std::strerror(errno);
							should_throw = true;
						}
					}
				});
				
				{
					auto const read_amt(::read(status_pipe[0], &status, sizeof(status)));
					switch (read_amt)
					{
						case -1:
							err = std::strerror(errno);
							should_throw = true;
							break;
						
						case 0:
						case (sizeof(status)):
							break;
						
						default:
							err = "Got an incomplete status object";
							should_throw = true;
							break;
					}
					
					::close(status_pipe[0]);
				}
				
				if (should_throw)
				{
					// Clean up if there was an error.
					process_open_fds([](auto const curr_spec, auto &fd_pair, auto const &trait){
						auto const parent_fd_idx(1 - trait.sp_fd_idx);
						::close(fd_pair[parent_fd_idx]);
					});
					
					::kill(pid, SIGTERM);
					throw std::runtime_error(err);
				}
				
				break;
			}
		}
		
		return {pid, io_fds[0][1], io_fds[1][0], io_fds[2][0], std::move(status)}; // Return stdin’s write end and the read end of the other two.
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
}


namespace libbio::detail {

	void subprocess_status::output_status(std::ostream &os, bool const detailed)
	{
		switch (execution_status)
		{
			case execution_status_type::no_error:
				return;

			case execution_status_type::file_descriptor_handling_failed:
				os << "File descriptor handling failed";
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

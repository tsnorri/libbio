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
	
	std::tuple <pid_t, int>
	open_subprocess_for_writing(char * const args[]) // nullptr-terminated list of arguments.
	{
		// Note that if this function is invoked simultaneously from different threads,
		// the subprocesses can leak file descriptors b.c. the ones resulting from the
		// call to pipe() may not have been closed or marked closed-on-exec.
		
		// Create a pipe for writing only for now.
		int fd[2];
		if (0 != ::pipe(fd))
			throw std::runtime_error(::strerror(errno));
		
		auto const pid(::fork());
		switch (pid)
		{
			case -1:
			{
				auto const *err(::strerror(errno));
				::close(fd[0]);
				::close(fd[1]);
				throw std::runtime_error(err);
				break;
			}
			
			case 0: // Child.
			{
				// Try to make the child continue when debugging.
				::signal(SIGTRAP, SIG_IGN);
				
				::close(STDOUT_FILENO);
				::close(STDERR_FILENO);
				
				if (
					-1 == ::dup2(fd[0], STDIN_FILENO)						// Reassign to stdin, closes the old descriptor.
					|| -1 == ::openat(STDOUT_FILENO, "/dev/null", O_WRONLY)
					|| -1 == ::openat(STDERR_FILENO, "/dev/null", O_WRONLY)
				)
				{
					throw std::runtime_error(::strerror(errno));
				}
				
				::close(fd[0]);
				::close(fd[1]);
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
				::close(fd[0]);
				
				// Enalbe close-on-exec for the writing end.
				if (-1 == ::fcntl(fd[1], F_SETFD, FD_CLOEXEC))
				{
					auto const *err(::strerror(errno));
					::close(fd[1]);
					throw std::runtime_error(err);
				}
			}
		}
		
		return {pid, fd[1]};
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

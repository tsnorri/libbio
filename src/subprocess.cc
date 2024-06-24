/*
 * Copyright (c) 2022-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <boost/container/static_vector.hpp>
#include <cstring>
#include <expected>
#include <fcntl.h>
#include <libbio/subprocess.hh>
#include <mutex>
#include <signal.h>
#include <stdexcept>
#include <sys/wait.h>
#include <unistd.h>

namespace bits		= libbio::bits;
namespace container	= boost::container;
namespace lb		= libbio;


#define MAKE_STATUS(EXECUTION_STATUS)					lb::subprocess_status{(EXECUTION_STATUS), __LINE__, errno}
#define EXIT_SUBPROCESS(EXECUTION_STATUS, EXIT_STATUS)	do_exit(status_fd, (EXIT_STATUS), (EXECUTION_STATUS), __LINE__, errno)


namespace {
	
	struct file_handle_status_reporter
	{
		int error{};
		
		constexpr operator bool() const { return !error; }
		
		void handle_error()
		{
			if (!error)
				error = errno;
		}
	};
	
	
	class file_handle final : public lb::file_handle
	{
	protected:
		file_handle_status_reporter	*m_sr{};
		
		void handle_close_error() override { m_sr->handle_error(); }
		
	public:
		explicit file_handle(file_handle_status_reporter &sr):
			lb::file_handle(),
			m_sr(&sr)
		{
		}
		
		file_handle(int fd, file_handle_status_reporter &sr):
			lb::file_handle(fd, true),
			m_sr(&sr)
		{
		}
		
		void assign(int const fd) { m_fd = fd; }
	};
	
	
	struct fork_result
	{
		pid_t										pid{-1};
		file_handle									status_handle;
		container::static_vector <file_handle, 3>	stdio_handles; // Makes things easier since clear() is available.
		
		explicit fork_result(lb::subprocess_handle_spec const handle_spec, file_handle_status_reporter &status_reporter):
			status_handle(status_reporter)
		{
		}
	};
	
	
	std::expected <fork_result, lb::subprocess_status>
	do_fork(lb::subprocess_handle_spec const handle_spec, file_handle_status_reporter &status_reporter)
	{
		fork_result parent_res(handle_spec, status_reporter);
		fork_result child_res(handle_spec, status_reporter);
		
		{
			int fds[2]{-1, -1};
			if (-1 == ::pipe(fds))
				return std::unexpected{MAKE_STATUS(lb::execution_status_type::file_descriptor_handling_failed)};
			
			parent_res.status_handle.assign(fds[0]);
			child_res.status_handle.assign(fds[1]);
		}
		
		auto const add_handle_if_needed([&](lb::subprocess_handle_spec const spec, bool const is_out_handle){
			int fds[2]{-1, -1};
			if (handle_spec & spec)
			{
				if (-1 == ::pipe(fds))
					throw MAKE_STATUS(lb::execution_status_type::file_descriptor_handling_failed);
				
				parent_res.stdio_handles.emplace_back(fds[1 - is_out_handle], status_reporter);
				child_res.stdio_handles.emplace_back(fds[0 + is_out_handle], status_reporter);
			}
		});
		
		try // This is the least cluttered approach.
		{
			add_handle_if_needed(lb::subprocess_handle_spec::STDIN, false);
			add_handle_if_needed(lb::subprocess_handle_spec::STDOUT, true);
			add_handle_if_needed(lb::subprocess_handle_spec::STDERR, true);
		}
		catch (lb::subprocess_status const &status)
		{
			return std::unexpected{status};
		}
		
		auto const pid(::fork());
		switch (pid)
		{
			case -1:
				return std::unexpected(MAKE_STATUS(lb::execution_status_type::fork_failed));
				break;
		
			// We close the other handle here.
			case 0: // Child
				child_res.pid = 0;
				return child_res;
		
			default: // Parent
				parent_res.pid = pid;
				return parent_res;
		}
	}
	
	
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
}


namespace libbio { namespace detail {
	
	subprocess_status
	open_subprocess(
		process_handle &ph,
		char const * const args[],
		subprocess_handle_spec const handle_spec,
		lb::file_handle *dst_handles
	) // nullptr-terminated list of arguments.
	{
		// Note that if this function is invoked simultaneously from different threads,
		// the subprocesses can leak file descriptors b.c. the ones resulting from the
		// call to pipe() may not have been closed or marked closed-on-exec.
		
		file_handle_status_reporter status_reporter;
		auto res_(do_fork(handle_spec, status_reporter));
		
		if (res_)
		{
			auto &res(*res_);
			if (0 == res.pid)
			{
				// Child.
				
				// Mark close-on-exec.
				auto const status_fd(res.status_handle.get());
				if (-1 == ::fcntl(status_fd, F_SETFD, FD_CLOEXEC))
					std::abort(); // Unable to report this error.
				
				// Try to make the child continue when debugging.
				::signal(SIGTRAP, SIG_IGN);
				
				// Open /dev/null if needed.
				auto dn_handle([&]() {
					if (3 == res.stdio_handles.size())
						return file_handle{-1, status_reporter};
					
					auto const fd(::open("/dev/null", O_RDWR));
					if (-1 == fd)
						EXIT_SUBPROCESS(execution_status_type::file_descriptor_handling_failed, 69); // EX_UNAVAILABLE in sysexits.h.
					
					return file_handle{fd, status_reporter};
				}());
				
				// Redirect I/O.
				{
					auto it(res.stdio_handles.begin());
					auto const handle_stdio_fh([&res, &it, &dn_handle, handle_spec, status_fd](lb::subprocess_handle_spec const spec, int const fd, int const oflags){
						if (handle_spec & spec)
						{
							if (-1 == ::dup2(it->get(), fd))
								EXIT_SUBPROCESS(execution_status_type::file_descriptor_handling_failed, 69);
							++it;
						}
						else
						{
							if (-1 == ::dup2(dn_handle.get(), fd))
								EXIT_SUBPROCESS(execution_status_type::file_descriptor_handling_failed, 69);
						}
					});
					
					handle_stdio_fh(lb::subprocess_handle_spec::STDIN, STDIN_FILENO, O_RDONLY);
					handle_stdio_fh(lb::subprocess_handle_spec::STDOUT, STDOUT_FILENO, O_WRONLY);
					handle_stdio_fh(lb::subprocess_handle_spec::STDERR, STDERR_FILENO, O_WRONLY);
				}
				
				// Close everything and check.
				res.stdio_handles.clear();
				if (!status_reporter)
					EXIT_SUBPROCESS(lb::execution_status_type::file_descriptor_handling_failed, 69);
				
				if (!dn_handle.close())
					EXIT_SUBPROCESS(lb::execution_status_type::file_descriptor_handling_failed, 69);
				
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
			}
			else
			{
				// Parent.
				auto const status_fd(res.status_handle.get());
				lb::subprocess_status status;
				
				{
					auto const read_amt(::read(status_fd, &status, sizeof(status)));
					switch (read_amt)
					{
						case -1:
							return MAKE_STATUS(execution_status_type::file_descriptor_handling_failed);
						
						case 0:
						case (sizeof(status)):
							break;
						
						default:
							errno = EBADMSG;
							return MAKE_STATUS(execution_status_type::file_descriptor_handling_failed);
					}
				}
				
				if (!status)
					return status;
				
				if (!res.status_handle.close())
					return MAKE_STATUS(execution_status_type::file_descriptor_handling_failed);
				
				ph = process_handle(res.pid);
				for (auto &&fh : res.stdio_handles)
					*dst_handles++ = std::move(fh);
				
				return status;
			}
		}
		
		// Fork failed.
		return res_.error();
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
	
	
	void subprocess_status::output_status(std::ostream &os, bool const detailed) const
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

		os << ". " << std::strerror(error) << '.';

		if (detailed)
			os << " (" << __FILE__ << ':' << line << ")";
	}
}

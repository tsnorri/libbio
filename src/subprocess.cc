/*
 * Copyright (c) 2022-2023 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <cstring>
#include <fcntl.h>
#include <libbio/subprocess.hh>
#include <memory_resource>
#include <mutex>
#include <stdexcept>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

namespace lb	= libbio;


#define EXIT_SUBPROCESS(EXECUTION_STATUS, EXIT_STATUS)	do { *status_ptr = subprocess_status{(EXECUTION_STATUS), __LINE__, errno}; ::_exit((EXIT_STATUS)); } while (false)


namespace {
	
	class shared_memory_resource final : public std::pmr::memory_resource
	{
		void *do_allocate(std::size_t const bytes, std::size_t const alignment) override;
		void do_deallocate(void *ptr, std::size_t const bytes, std::size_t const alignment) noexcept override;
		bool do_is_equal(std::pmr::memory_resource const &other) const noexcept override { return this == &other; }
	};
	
	
	void *shared_memory_resource::do_allocate(std::size_t const bytes, std::size_t const alignment)
	{
		auto *retval(::mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0));
		
		if (MAP_FAILED == retval)
			throw lb::shared_memory_allocation_error{errno};
		
		if (0 != uintptr_t(retval) % alignment)
			throw std::bad_alloc{};
		
		return retval;
	}
	
	
	void shared_memory_resource::do_deallocate(void *ptr, std::size_t const bytes, std::size_t const alignment) noexcept
	{
		::munmap(ptr, bytes);
	}
	
	
	std::mutex								g_pool_mutex{};
	std::uint64_t							g_pool_resource_use_count{};
	shared_memory_resource					g_shm_resource{};				// Stateless.
	
	auto &get_pool_resource()
	{
		static std::pmr::unsynchronized_pool_resource pool_resource{&g_shm_resource};
		return pool_resource;
	}
}


namespace libbio { namespace detail {
	
	void deallocate_subprocess_object_memory(void *ptr, std::size_t const size, std::size_t const alignment)
	{
		std::lock_guard lock{g_pool_mutex};
		
		auto &pool_resource(get_pool_resource());
		pool_resource.deallocate(ptr, size, alignment);
		
		libbio_assert_lt(0, g_pool_resource_use_count);
		--g_pool_resource_use_count;
		if (0 == g_pool_resource_use_count)
			pool_resource.release(); // Release all allocated memory.
	}
}}


namespace {
	
	template <typename t_type, typename ... t_args>
	lb::subprocess_object_ptr <t_type> make_subprocess_object(t_args && ... args)
	{
		void *dst{};
		
		{
			std::lock_guard lock{g_pool_mutex};
			auto &pool_resource(get_pool_resource());
			++g_pool_resource_use_count;
			dst = pool_resource.allocate(sizeof(t_type), alignof(t_type));
		}
		
		t_type *obj{};
		try
		{
			obj = new (dst) t_type(std::forward <t_args>(args)...);
		}
		catch (...)
		{
			// Avoid leaking memory.
			lb::detail::deallocate_subprocess_object_memory(dst, sizeof(t_type), alignof(t_type));
			throw;
		}
		
		return lb::subprocess_object_ptr <t_type>{obj};
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
	
	
	std::tuple <pid_t, int, int, int, subprocess_object_ptr <subprocess_status>>
	open_subprocess(char const * const args[], subprocess_handle_spec const handle_spec) // nullptr-terminated list of arguments.
	{
		// Note that if this function is invoked simultaneously from different threads,
		// the subprocesses can leak file descriptors b.c. the ones resulting from the
		// call to pipe() may not have been closed or marked closed-on-exec.
		
		// Status object for communicating errors to the parent process.
		auto status_ptr(make_subprocess_object <subprocess_status>());
		
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
				
				process_all_fds([handle_spec, &status_ptr](auto const curr_spec, auto &fd_pair, auto const &trait){
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
		
		return {pid, fds[0][1], fds[1][0], fds[2][0], std::move(status_ptr)}; // Return stdin’s write end and the read end of the other two.
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
				
				throw std::runtime_error(::strerror(errno));
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

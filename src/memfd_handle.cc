/*
 * Copyright (c) 2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <atomic>
#include <fcntl.h>					// O_RDWR etc.
#include <format>
#include <libbio/memfd_handle.hh>
#include <sys/mman.h>				// ::shm_open


namespace libbio {
	
#ifdef __linux__
	memfd_handle open_anonymous_memory_file()
	{
		auto const fd(::memfd_create("libbio-memfd-handle", MFD_CLOEXEC));
		if (-1 == fd)
			throw std::runtime_error(::strerror(errno));
		
		return memfd_handle{fd};
	}
#else
	memfd_handle open_anonymous_memory_file()
	{
		static std::atomic_uint32_t counter{};
		auto const idx{counter.fetch_add(1, std::memory_order_relaxed)};
		
		memfd_handle retval;
		
		auto name(std::format("/libbio-anonymous-{}-{}", ::getpid(), idx));
		auto const fd(::shm_open(name.data(), O_RDWR | O_CREAT | O_EXCL));
		if (-1 == fd)
			throw std::runtime_error(::strerror(errno));
		
		// At least macOS sets FD_CLOEXEC by itself.
		
		// Unlink to make the name available. We still have the file open so
		// it should not disappear.
		if (-1 == ::shm_unlink(name.data()))
			throw std::runtime_error(::strerror(errno));
		
		retval.fd = fd;
		return retval;
	}
#endif
}

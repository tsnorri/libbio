/*
 * Copyright (c) 2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_MEMFD_HANDLE_HH
#define LIBBIO_MEMFD_HANDLE_HH

#include <algorithm>	// std::swap
#include <unistd.h>		// ::close


namespace libbio {
	
	struct memfd_handle
	{
		int fd{-1};
		
		memfd_handle() = default;
		
		explicit memfd_handle(int fd_):
			fd(fd_)
		{
		}
		
		~memfd_handle() { if (-1 != fd) ::close(fd); }
		
		memfd_handle(memfd_handle const &) = delete;
		memfd_handle &operator=(memfd_handle const &) = delete;
		
		inline memfd_handle(memfd_handle &&other);
		inline memfd_handle &operator=(memfd_handle &&other) &;
	};
	
	
	memfd_handle open_anonymous_memory_file();
	
	
	memfd_handle::memfd_handle(memfd_handle &&other)
	{
		using std::swap;
		swap(fd, other.fd);
	}
	
	memfd_handle &memfd_handle::operator=(memfd_handle &&other) &
	{
		using std::swap;
		if (this != &other)
			swap(fd, other.fd);
		return *this;
	}
}

#endif

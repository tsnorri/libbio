/*
 * Copyright (c) 2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_MMAP_HANDLE_HH
#define LIBBIO_MMAP_HANDLE_HH

#include <algorithm>	// std::swap
#include <cstddef>		// std::byte, std::size_t
#include <sys/mman.h>


namespace libbio {
	
	struct mmap_handle
	{
		std::byte	*data{};
		std::size_t	size{};
		
		mmap_handle() = default;
		
		explicit mmap_handle(std::byte *data_, std::size_t size_):
			data(data_),
			size(size_)
		{
		}
		
		~mmap_handle() { if (data) ::munmap(data, size); }
		
		mmap_handle(mmap_handle const &) = delete;
		mmap_handle &operator=(mmap_handle const &) = delete;
		
		inline mmap_handle(mmap_handle &&other);
		inline mmap_handle &operator=(mmap_handle &&other) &;
	};
	
	
	mmap_handle::mmap_handle(mmap_handle &&other)
	{
		using std::swap;
		swap(data, other.data);
		swap(size, other.size);
	}
	
	
	mmap_handle &mmap_handle::operator=(mmap_handle &&other) &
	{
		using std::swap;
		if (this != &other)
		{
			swap(data, other.data);
			swap(size, other.size);
		}
		return *this;
	}
}

#endif

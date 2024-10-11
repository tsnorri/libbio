/*
 * Copyright (c) 2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <libbio/circular_buffer.hh>
#include <libbio/memfd_handle.hh>


namespace libbio {
	
	int circular_buffer::s_page_size{::getpagesize()};
	
	
	void circular_buffer::allocate(std::size_t page_count)
	{
		// Allocate the memory.
		auto const size(page_count * s_page_size);
		auto handle(open_anonymous_memory_file());
		if (0 != ::ftruncate(handle.fd, page_count * s_page_size))
			throw std::runtime_error(::strerror(errno));
		
		// Get a memory region.
		auto const mapping_size(2 * size);
		auto region(static_cast <std::byte *>(::mmap(nullptr, mapping_size, PROT_NONE, MAP_ANON | MAP_PRIVATE, -1, 0)));
		if (MAP_FAILED == region)
			throw std::runtime_error(::strerror(errno));
		
		// Map the memory file twice to the region.
		if (MAP_FAILED == ::mmap(region, size, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_SHARED, handle.fd, 0))
			throw std::runtime_error(::strerror(errno));
		
		if (MAP_FAILED == ::mmap(region + size, size, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_SHARED, handle.fd, 0))
			throw std::runtime_error(::strerror(errno));
		
		m_handle = mmap_handle(region, mapping_size);
		m_size = size;
		m_mask = size - 1;
		m_base = region;
		m_begin = 0;
		m_end = 0;
	}
}

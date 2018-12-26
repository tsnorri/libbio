/*
 * Copyright (c) 2017-2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <algorithm>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <libbio/assert.hh>
#include <libbio/mmap_handle.hh>
#include <stdexcept>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>


namespace libbio {
	
	mmap_handle::~mmap_handle()
	{
		if (m_mapped_size)
		{
			libbio_assert(m_content);
			if (-1 == munmap(m_content, m_mapped_size))
				std::cerr << "munmap: " << strerror(errno) << std::endl;
		}
	}
	
	
	void mmap_handle::close()
	{
		if (m_mapped_size)
		{
			libbio_assert(m_content);
			if (-1 == munmap(m_content, m_mapped_size))
				throw std::runtime_error(strerror(errno));
			
			m_mapped_size = 0;
		}
	}
	
	
	void mmap_handle::open(int fd)
	{
		struct stat sb{};
		if (-1 == fstat(fd, &sb))
			throw std::runtime_error(strerror(errno));
		
		if (!S_ISREG(sb.st_mode))
			throw std::runtime_error("Trying to memory map a non-regular file");
		
		m_content = static_cast <char *>(mmap(0, sb.st_size, PROT_READ, MAP_FILE | MAP_PRIVATE, fd, 0));
		if (MAP_FAILED == m_content)
			throw std::runtime_error(strerror(errno));
		
		m_mapped_size = sb.st_size;
	}
	
	
	void mmap_handle::open(std::string const &path)
	{
		char const *c_str(path.c_str());
		int fd(::open(c_str, O_RDONLY));
		if (-1 == fd)
			throw std::runtime_error(strerror(errno));
			
		open(fd);
		if (-1 == ::close(fd))
			throw std::runtime_error(strerror(errno));
		
		m_path = path;
	}
	
	
	std::ostream &operator<<(std::ostream &stream, mmap_handle const &handle)
	{
		std::string_view const sv(handle.m_content, std::min(16UL, handle.m_mapped_size));
		stream << "path: '" << handle.m_path << "' mapped size: " << handle.m_mapped_size << " content: '" << sv;
		if (16 < handle.m_mapped_size)
			stream << "…";
		stream << '\'';
		return stream;
	}
}

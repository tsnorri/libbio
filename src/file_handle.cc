/*
 * Copyright (c) 2021 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <cstdio>
#include <cstring>
#include <iostream>
#include <libbio/file_handle.hh>
#include <unistd.h>


namespace libbio {
	
	file_handle::~file_handle()
	{
		if (-1 != m_fd)
		{
			auto const res(::close(m_fd));
			if (0 != res)
				std::cerr << "ERROR: unable to close file handle: " << std::strerror(errno) << '\n';
		}
	}
	
	
	void file_handle::close()
	{
		// FIXME: should we check the status?
		::close(m_fd);
		m_fd = -1;
	}
	
	
	std::size_t file_handle::seek(std::size_t const pos, int const whence)
	{
		auto const res(::lseek(m_fd, pos, whence));
		if (-1 == res)
			throw std::runtime_error(std::strerror(errno));
		
		return res;
	}
	
	
	std::size_t file_handle::read(std::size_t const len, char * const dst)
	{
		auto const res(::read(m_fd, dst, len));
		if (-1 == res)
			throw std::runtime_error(std::strerror(errno));
		return res;
	}
	
	
	void file_handle::truncate(std::size_t const len)
	{
		auto const res(::ftruncate(m_fd, len));
		if (-1  == res)
			throw std::runtime_error(std::strerror(errno));
	}
	
	
	void file_handle::stat(struct ::stat &sb)
	{
		auto const res(::fstat(m_fd, &sb));
		if (-1 == res)
			throw std::runtime_error(std::strerror(errno));
	}
}

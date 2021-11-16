/*
 * Copyright (c) 2021 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <cstdio>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <libbio/file_handle.hh>


namespace libbio {
	
	file_handle::~file_handle()
	{
		if (-1 != m_fd)
		{
			auto const res(close(m_fd));
			if (0 != res)
				std::cerr << "ERROR: unable to close file handle: " << std::strerror(errno) << '\n';
		}
	}
	
	
	void file_handle::seek(std::size_t const pos)
	{
		auto const res(::lseek(m_fd, pos, SEEK_SET));
		if (-1 == res)
			throw std::runtime_error(std::strerror(errno));
		else if (res != pos)
			throw std::runtime_error("Unable to seek");
	}
	
	
	std::size_t file_handle::read(std::size_t const len, char * const dst)
	{
		auto const res(::read(m_fd, dst, len));
		if (-1 == res)
			throw std::runtime_error(std::strerror(errno));
		return res;
	}
}

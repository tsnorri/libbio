/*
 * Copyright (c) 2021–2024 Tuukka Norri
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
		if (-1 != m_fd && m_should_close)
		{
			if (0 != ::close(m_fd))
				this->handle_close_error();
		}
	}
	
	
	void file_handle::handle_close_error()
	{
		std::cerr << "ERROR: unable to close file handle " << m_fd << ": " << std::strerror(errno) << '\n';
	}
	
	
	bool file_handle::close()
	{
		if (0 != ::close(m_fd))
			return false;
		m_fd = -1;
		return true;
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
		while (true)
		{
			auto const res(::read(m_fd, dst, len));
			if (-1 == res)
			{
				if (EINTR == errno)
					continue;

				throw std::runtime_error(std::strerror(errno));
			}
			
			return res;
		}
	}
	
	
	void file_handle::truncate(std::size_t const len)
	{
		while (true)
		{
			auto const res(::ftruncate(m_fd, len));
			if (-1  == res)
			{
				if (EINTR == errno)
					continue;
				
				throw std::runtime_error(std::strerror(errno));
			}
			
			return;
		}
	}
	
	
	void file_handle::stat(struct ::stat &sb) const
	{
		auto const res(::fstat(m_fd, &sb));
		if (-1 == res)
			throw std::runtime_error(std::strerror(errno));
	}
	
	
	std::size_t file_handle::io_op_blocksize() const
	{
		struct stat sb{};
		stat(sb);
		return sb.st_blksize;
	}
}

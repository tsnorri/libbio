/*
 * Copyright (c) 2021–2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <libbio/file_handle.hh>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>


namespace {

	constexpr inline bool should_output_error_in_destructor()
	{
#if defined(LIBBIO_NDEBUG) && LIBBIO_NDEBUG
		return false;
#else
		return true;
#endif
	}
}


namespace libbio {

	file_handle_::~file_handle_()
	{
		if (-1 != m_fd && m_should_close && !close())
		{
			if constexpr (should_output_error_in_destructor())
				std::cerr << "ERROR: unable to close file handle " << m_fd << ": " << std::strerror(errno) << '\n';
		}
	}


	bool file_handle_::close()
	{
		if (0 != ::close(m_fd))
			return false;
		m_fd = -1;
		return true;
	}


	std::size_t file_handle_::seek(std::size_t const pos, int const whence)
	{
		auto const res(::lseek(m_fd, pos, whence));
		if (-1 == res)
			throw std::runtime_error(std::strerror(errno));

		return res;
	}


	std::size_t file_handle_::read(std::size_t const len, std::byte * const dst)
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


	std::size_t file_handle_::write(char const *data, std::size_t const len)
	{
		while (true)
		{
			auto const res(::write(m_fd, data, len));
			if (-1 == res)
			{
				if (EINTR == errno)
					continue;

				throw std::runtime_error(std::strerror(errno));
			}

			return res;
		}
	}


	void file_handle_::truncate(std::size_t const len)
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


	void file_handle_::stat(struct ::stat &sb) const
	{
		auto const res(::fstat(m_fd, &sb));
		if (-1 == res)
			throw std::runtime_error(std::strerror(errno));
	}


	std::size_t file_handle_::io_op_blocksize() const
	{
		struct stat sb{};
		stat(sb);
		return sb.st_blksize;
	}
}

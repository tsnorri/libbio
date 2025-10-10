/*
 * Copyright (c) 2021–2025 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_FILE_HANDLE_HH
#define LIBBIO_FILE_HANDLE_HH

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <sys/stat.h>
#include <utility>


namespace libbio {

	struct reading_handle
	{
		virtual ~reading_handle() {}
		std::size_t read(std::size_t const len, char *dst) { return read(len, reinterpret_cast <std::byte *>(dst)); }
		virtual std::size_t read(std::size_t const len, std::byte *dst) = 0; // throws
	};


	class file_handle : public reading_handle
	{
	public:
		typedef int	file_descriptor_type;

	protected:
		file_descriptor_type			m_fd{-1};
		bool							m_should_close{};

	public:
		file_handle() = default;

		// Takes ownership.
		explicit file_handle(file_descriptor_type fd, bool should_close = true) noexcept:
			m_fd(fd),
			m_should_close(should_close)
		{
		}

		using reading_handle::read;

		// Copying not allowed.
		file_handle(file_handle const &) = delete;
		file_handle &operator=(file_handle const &) = delete;

		inline file_handle(file_handle &&other);
		inline file_handle &operator=(file_handle &&other);

		virtual ~file_handle();

		file_descriptor_type get() const { return m_fd; }

		// Releases ownership.
		file_descriptor_type release() { auto const retval(m_fd); m_fd = -1; return retval; }

		std::size_t seek(std::size_t const pos, int const whence = SEEK_SET); // throws
		std::size_t read(std::size_t const len, std::byte *dst) override; // throws
		std::size_t write(char const *data, std::size_t const len);
		void truncate(std::size_t const len); // throws
		void stat(struct ::stat &sb) const; // throws
		std::size_t io_op_blocksize() const; // throws

		bool close();
	};


	file_handle::file_handle(file_handle &&other)
	{
		using std::swap;
		swap(m_fd, other.m_fd);
		swap(m_should_close, other.m_should_close);
	}


	file_handle &file_handle::operator=(file_handle &&other)
	{
		if (this != &other)
		{
			using std::swap;
			swap(m_fd, other.m_fd);
			swap(m_should_close, other.m_should_close);
		}
		return *this;
	}
}

#endif

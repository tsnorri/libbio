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
		virtual void prepare() {}
		std::size_t read(std::size_t const len, char *dst) { return read(len, reinterpret_cast <std::byte *>(dst)); }
		virtual std::size_t read(std::size_t const len, std::byte *dst) = 0; // throws
		virtual std::size_t io_op_blocksize() const = 0;
		virtual void finish() {};
	};


	class file_handle_ : public reading_handle
	{
	public:
		typedef int	file_descriptor_type;

	protected:
		file_descriptor_type			m_fd{-1};
		bool							m_should_close{};

	public:
		file_handle_() = default;

		// Takes ownership.
		explicit file_handle_(file_descriptor_type fd, bool should_close = true) noexcept:
			m_fd(fd),
			m_should_close(should_close)
		{
		}

		using reading_handle::read;

		// Copying not allowed.
		file_handle_(file_handle_ const &) = delete;
		file_handle_ &operator=(file_handle_ const &) = delete;

		inline file_handle_(file_handle_ &&other);
		inline file_handle_ &operator=(file_handle_ &&other);

		virtual ~file_handle_();

		file_descriptor_type get() const { return m_fd; }

		// Releases ownership.
		file_descriptor_type release() { auto const retval(m_fd); m_fd = -1; return retval; }

		std::size_t seek(std::size_t const pos, int const whence = SEEK_SET); // throws
		std::size_t read(std::size_t const len, std::byte *dst) override; // throws
		std::size_t write(char const *data, std::size_t const len);
		void truncate(std::size_t const len); // throws
		void stat(struct ::stat &sb) const; // throws
		std::size_t io_op_blocksize() const override; // throws

		bool close();
	};


	class file_handle final : public file_handle_
	{
		using file_handle_::file_handle_;
	};


	file_handle_::file_handle_(file_handle_ &&other)
	{
		using std::swap;
		swap(m_fd, other.m_fd);
		swap(m_should_close, other.m_should_close);
	}


	file_handle_ &file_handle_::operator=(file_handle_ &&other)
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

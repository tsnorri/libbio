/*
 * Copyright (c) 2025 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_GZIP_READ_HANDLE_HH
#define LIBBIO_GZIP_READ_HANDLE_HH

#include <cstddef>
#include <libbio/circular_buffer.hh>
#include <libbio/file_handle.hh>
#include <zlib.h>


namespace libbio {

	class gzip_reading_handle final : public reading_handle
	{
	private:
		constexpr static std::size_t block_size{32768};

	private:
		file_handle			*m_gzip_handle{}; // Not owned.
		z_stream			m_stream{};
		circular_buffer		m_input_buffer;
		std::size_t			m_io_op_blocksize{};

	public:
		~gzip_reading_handle() { ::inflateEnd(&m_stream); }

		void prepare() override; // Call once before using.
		void set_gzip_input_handle(file_handle &handle); // Set the next file to be processed.
		void finish() override; // Call after processing the file.
		std::size_t read(std::size_t const len, std::byte *dst) override; // Try to read some data.
		std::size_t io_op_blocksize() const override { return block_size; }
	};
}

#endif

/*
 * Copyright (c) 2025 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <cstddef>
#include <libbio/assert.hh>
#include <libbio/file_handle.hh>
#include <libbio/gzip_read_handle.hh>
#include <stdexcept>
#include <zlib.h>


namespace libbio {

	void gzip_reading_handle::prepare()
	{
		auto const res{::inflateInit2(&m_stream, 16 + MAX_WBITS)};
		if (Z_OK != res)
			throw std::runtime_error("Problem with inflateInit2");
	}


	void gzip_reading_handle::set_gzip_input_handle(file_handle &handle)
	{
		m_gzip_handle = &handle;

		m_io_op_blocksize = handle.io_op_blocksize() ?: 32768U;
		if (m_input_buffer.size() < m_io_op_blocksize)
		{
			// Make sure that at least two blocks fit into the buffer.
			auto const page_count((3 * m_io_op_blocksize - 1) / m_input_buffer.page_size());
			m_input_buffer.allocate(page_count);
		}
		m_input_buffer.clear();

		m_stream.avail_in = 0;
		m_stream.next_in = nullptr;
	}


	void gzip_reading_handle::finish()
	{
		auto const res{::inflateReset(&m_stream)};
		if (Z_OK != res)
			throw std::runtime_error("Problem with inflateReset");
	}


	std::size_t gzip_reading_handle::read(std::size_t const requested_length, std::byte *dst)
	{
		if (!requested_length)
			return 0;

		// FIXME: The fact that no data can be read should be taken into account in the function signature. The return type should be changed to e.g. std::expected. In that case we should not throw on error.
		std::size_t prev_read_amount{};
		m_stream.avail_out = requested_length;
		m_stream.next_out = reinterpret_cast <unsigned char *>(dst);
		while (true)
		{
			auto prev_avail_in{m_stream.avail_in};
			while (m_stream.avail_in && m_stream.avail_out)
			{
				auto const res{::inflate(&m_stream, Z_SYNC_FLUSH)};
				switch (res)
				{
					case Z_OK:
					{
						// Check if we were able to read something over all iterations.
						auto const read_amount{requested_length - m_stream.avail_out};
						if (read_amount == prev_read_amount)
						{
							if (read_amount)
								return read_amount;
							throw std::runtime_error("Not enough memory in the provided buffer");
						}

						break;
					}

					case Z_STREAM_END:
					{
						auto const read_amount{requested_length - m_stream.avail_out};
						return read_amount;
					}

					case Z_BUF_ERROR:
						throw std::runtime_error("Not enough memory in the provided buffer");

					case Z_NEED_DICT:
					case Z_STREAM_ERROR:
					case Z_DATA_ERROR:
					case Z_MEM_ERROR:
					default:
						throw std::runtime_error("Problem with inflate");
				}

				// Update the input buffer state.
				auto const processed_input_bytes{prev_avail_in - m_stream.avail_in};
				m_input_buffer.add_to_available(processed_input_bytes);
				prev_avail_in = m_stream.avail_in;

				// Update the reading position.
				auto reading_range{m_input_buffer.reading_range_()};
				m_stream.avail_in = reading_range.size();
				m_stream.next_in = reinterpret_cast <unsigned char *>(reading_range.it);
			}

			// Add more data to the input buffer.
			auto const available_size((m_input_buffer.size_available() / m_io_op_blocksize) * m_io_op_blocksize);
			libbio_always_assert_lt(0, available_size);
			auto writing_range{m_input_buffer.writing_range()};
			auto const read_size{m_gzip_handle->read(available_size, writing_range.it)};
			m_input_buffer.add_to_occupied(read_size);
		}
	}
}

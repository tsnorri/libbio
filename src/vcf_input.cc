/*
 * Copyright (c) 2017-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <algorithm>
#include <cstddef>
#include <ios>
#include <istream>
#include <libbio/assert.hh>
#include <libbio/vcf/variant.hh>
#include <libbio/vcf/vcf_input.hh>
#include <libbio/vcf/vcf_reader.hh>
#include <string_view>


namespace {

	void stream_read(std::istream &stream, char *dst, std::size_t const len)
	{
		try
		{
			stream.read(dst, len);
		}
		catch (std::ios_base::failure const &exc)
		{
			if (!stream.eof())
				throw (exc);
		}
	}
}


namespace libbio::vcf {

	void empty_input::fill_buffer(reader &vcf_reader)
	{
		vcf_reader.set_buffer_start(nullptr);
		vcf_reader.set_buffer_end(nullptr);
		vcf_reader.set_eof(nullptr);
	}


	void stream_input_base::reader_will_take_input()
	{
		if (0 == m_buffer.size())
			m_buffer.resize(65536);
	}


	void stream_input_base::fill_buffer(reader &vcf_reader)
	{
		auto &is(stream());
		// Copy the remainder to the beginning.
		if (m_pos + 1 < m_len)
		{
			char *data_start(m_buffer.data());
			char *start(data_start + m_pos + 1);
			char *end(data_start + m_len);
			std::copy(start, end, data_start);
			m_len -= m_pos + 1;
		}
		else
		{
			m_len = 0;
		}

		// Read until there's at least one newline in the buffer.
		while (true)
		{
			char *data_start(m_buffer.data());
			char *data(data_start + m_len);

			std::size_t space(m_buffer.size() - m_len);
			stream_read(is, data, space);
			std::size_t const read_len(is.gcount());
			m_len += read_len;

			if (is.eof())
			{
				m_pos = m_len;
				auto const end(data_start + m_len);
				vcf_reader.set_buffer_start(data_start);
				vcf_reader.set_buffer_end(end);
				vcf_reader.set_eof(end);
				return;
			}

			// Try to find the last newline in the new part.
			std::string_view sv(data, read_len);
			m_pos = sv.rfind('\n');
			if (std::string_view::npos != m_pos)
			{
				m_pos += (data - data_start);
				vcf_reader.set_buffer_start(data_start);
				vcf_reader.set_buffer_end(data_start + m_pos + 1);
				vcf_reader.set_eof(nullptr);
				return;
			}

			libbio_assert_lt(0, m_buffer.size());
			m_buffer.resize(2 * m_buffer.size());
		}
	}


	void mmap_input::fill_buffer(reader &vcf_reader)
	{
		auto const begin(m_handle.data());
		auto const end(begin + m_handle.size());

		vcf_reader.set_buffer_start(begin);
		vcf_reader.set_buffer_end(end);
		vcf_reader.set_eof(end);
	}
}

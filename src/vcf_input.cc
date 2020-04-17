/*
 * Copyright (c) 2017-2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <libbio/vcf/variant.hh>
#include <libbio/vcf/vcf_input.hh>
#include <libbio/vcf/vcf_reader.hh>


namespace ios = boost::iostreams;


namespace libbio::vcf {
	
	void stream_input_base::reset_to_first_variant_offset()
	{
		stream_reset();
		m_len = 0;
		m_pos = 0;
	}
	
	
	bool stream_input_base::getline(std::string_view &dst)
	{
		bool const st(stream_getline());
		if (!st)
			return false;
		
		dst = std::string_view(m_buffer.data(), m_buffer.size());
		return true;
	}
	
	
	void stream_input_base::store_first_variant_offset(std::size_t const lineno)
	{
		input::store_first_variant_offset(lineno);
		m_first_variant_offset = stream_tellg();
	}
	
	
	void stream_input_base::fill_buffer(reader &vcf_reader)
	{
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
			char *data_start((char *) m_buffer.data()); // FIXME: hack, my libc++ does not yet have the non-const data().
			char *data(data_start + m_len);
		
			std::size_t space(m_buffer.size() - m_len);
			stream_read(data, space);
			std::size_t const read_len(stream_gcount());
			m_len += read_len;
		
			if (stream_eof())
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
				return;
			}
			
			m_buffer.resize(2 * m_buffer.size());
		}
	}
	
	
	void mmap_input::reset_range()
	{
		m_range_start_offset = 0;
		m_range_length = m_handle->size();
	}
	
	
	bool mmap_input::getline(std::string_view &dst)
	{
		auto const size(m_handle->size());
		libbio_assert(m_pos <= size);
		
		auto const sv(m_handle->operator std::string_view());
		auto const nl_pos(sv.find('\n', m_pos));
		if (std::string_view::npos == nl_pos)
			return false;
		
		// Found the newline, update the variables.
		dst = sv.substr(m_pos, nl_pos - m_pos);
		m_pos = 1 + nl_pos;
		return true;
	}
	
	
	void mmap_input::store_first_variant_offset(std::size_t const lineno)
	{
		input::store_first_variant_offset(lineno);

		m_first_variant_offset = m_pos;
		m_range_length = m_handle->size() - m_pos;
		m_range_start_offset = m_first_variant_offset;
		m_range_start_lineno = m_first_variant_lineno;
	}
	
	
	void mmap_input::fill_buffer(reader &vcf_reader)
	{
		auto const bytes(m_handle->data());
		auto const buffer_start(bytes + m_range_start_offset);
		auto const buffer_end(bytes + m_range_start_offset + m_range_length);
		
		vcf_reader.set_lineno(m_range_start_lineno);
		vcf_reader.set_buffer_start(buffer_start);
		vcf_reader.set_buffer_end(buffer_end);
		vcf_reader.set_eof(buffer_end);
	}
}

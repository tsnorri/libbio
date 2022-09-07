/*
 * Copyright (c) 2022 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <libbio/bed_reader.hh>
#include <libbio/file_handling.hh>
#include <libbio/utility/misc.hh>

namespace lb	= libbio;


namespace libbio {
	
	void bed_reader::read_regions(char const *path, bed_reader_delegate &delegate)
	{
		if (!path)
		{
			delegate.bed_reader_did_finish();
			return;
		}

		lb::file_istream stream;
		lb::open_file_for_reading(path, stream);
	
		m_lineno = 0;
		while (std::getline(stream, m_buffer))
		{
			++m_lineno;
			std::string_view const sv(m_buffer);
			
			m_curr_pos = 0;
			
			auto const chr_id(read_next_field(sv, false, delegate));
			auto const begin_str(read_next_field(sv, false, delegate));
			auto const end_str(read_next_field(sv, true, delegate));
		
			std::size_t begin{};
			std::size_t end{};
		
			if (! (parse_integer(begin_str, begin) && parse_integer(end_str, end)))
				delegate.bed_reader_reported_error(m_lineno);
		
			delegate.bed_reader_found_region(chr_id, begin, end);
		}
		
		delegate.bed_reader_did_finish();
	}
	
	
	std::string_view bed_reader::read_next_field(std::string_view const rec, bool const is_last, bed_reader_delegate &delegate)
	{
		auto const tab_pos(rec.find('\t', m_curr_pos));
		if (std::string::npos == tab_pos && !is_last)
			delegate.bed_reader_reported_error(m_lineno);
	
		auto const retval(rec.substr(m_curr_pos, tab_pos - m_curr_pos));
		m_curr_pos = 1 + tab_pos;
		
		return retval;
	}
}

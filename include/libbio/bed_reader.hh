/*
 * Copyright (c) 2022 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_BED_READER_HH
#define LIBBIO_BED_READER_HH

#include <istream>
#include <string>
#include <string_view>


namespace libbio {
	
	struct bed_reader_delegate
	{
		virtual ~bed_reader_delegate() {}
		virtual void bed_reader_reported_error(std::size_t const lineno) = 0;
		
		// Reports a half-open interval.
		virtual void bed_reader_found_region(std::string_view const chr_id, std::size_t const begin, std::size_t const end) = 0;
		
		virtual void bed_reader_did_finish() {}
	};
	
	
	// A very simple BED file reader,
	// currently handles only the first three fields, namely chromosome identifier and the region boundaries.
	class bed_reader
	{
	protected:
		std::string	m_buffer;
		std::size_t	m_lineno{};
		std::size_t	m_curr_pos{};
	
	protected:
		std::string_view read_next_field(std::string_view const rec, bool const is_last, bed_reader_delegate &delegate);
		
	public:
		void read_regions(char const *path, bed_reader_delegate &delegate);
	};
	
}

#endif

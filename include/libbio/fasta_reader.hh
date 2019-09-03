/*
 * Copyright (c) 2016-2019 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_FASTA_READER_HH
#define LIBBIO_FASTA_READER_HH

#include <iostream>
#include <libbio/cxxcompat.hh>
#include <libbio/mmap_handle.hh>


namespace libbio { namespace detail {
}}


namespace libbio {
	
	class fasta_reader;
	
	
	struct fasta_reader_delegate
	{
		virtual bool handle_comment_line(fasta_reader &reader, std::string_view const &sv) = 0;
		virtual bool handle_identifier(fasta_reader &reader, std::string_view const &sv) = 0;
		virtual bool handle_sequence_line(fasta_reader &reader, std::string_view const &sv) = 0;
	};
	
	
	class fasta_reader
	{
	public:
		typedef mmap_handle <char> handle_type;
		
	protected:
		struct fsm
		{
			char const	*p{nullptr};
			char const	*pe{nullptr};
			char const	*eof{nullptr};
		};
		
	protected:
		char const	*m_line_start{};
		fsm			m_fsm;
		std::size_t	m_lineno{};
	
	protected:
		void report_unexpected_character(int const current_state) const;
		void report_unexpected_eof(int const current_state) const;
		void output_buffer_end() const;
		
	public:
		bool parse(handle_type &handle, fasta_reader_delegate &delegate);
	};
}

#endif

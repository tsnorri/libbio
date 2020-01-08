/*
 * Copyright (c) 2019 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <libbio/fasta_reader.hh>


%% machine fasta_parser;
%% write data;

namespace
{
	enum class line_type
	{
		NONE,
		HEADER,
		SEQUENCE,
		COMMENT
	};
}


namespace libbio
{
	void fasta_reader::report_unexpected_character(int const current_state) const
	{
		std::cerr
		<< "Unexpected character '" << *m_fsm.p << "' at " << m_lineno << ':' << (m_fsm.p - m_line_start)
		<< ", state " << current_state << '.' << std::endl;
		output_buffer_end();
		abort();
	}
	
	
	void fasta_reader::report_unexpected_eof(int const current_state) const
	{
		std::cerr
		<< "Unexpected EOF at " << m_lineno << ':' << (m_fsm.p - m_line_start)
		<< ", state " << current_state << '.' << std::endl;
		output_buffer_end();
		abort();
	}
	
	
	void fasta_reader::output_buffer_end() const
	{
		auto const start(m_fsm.p - m_line_start < 128 ? m_line_start : m_fsm.p - 128);
		auto const len(m_fsm.p - start);
		std::string_view buffer_end(start, len);
		std::cerr
		<< "** Last " << len << " charcters from the buffer:" << std::endl
		<< buffer_end << std::endl;
	}
	
	
	bool fasta_reader::parse(handle_type &handle, fasta_reader_delegate &delegate)
	{
		bool retval(true);
		int cs(0);
		line_type current_line_type(line_type::NONE);
		
		m_fsm.p = handle.data();
		m_fsm.pe = m_fsm.p + handle.size();
		m_fsm.eof = m_fsm.pe;
		
		m_line_start = m_fsm.p;
		m_lineno = 0;
		
		%%{
			variable p		m_fsm.p;
			variable pe		m_fsm.pe;
			variable eof	m_fsm.eof;
			
			action comment_start {
				current_line_type = line_type::COMMENT; // Set the line type for the EOF handler.
				++m_lineno;
				m_line_start = fpc;
			}

			action header_start {
				current_line_type = line_type::HEADER; // Set the line type for the EOF handler.
				++m_lineno;
				m_line_start = fpc;
			}
			
			action sequence_start {
				current_line_type = line_type::SEQUENCE; // Set the line type for the EOF handler.
				++m_lineno;
				m_line_start = fpc;
			}
			
			action comment {
				current_line_type = line_type::NONE;
				std::string_view const sv(1 + m_line_start, fpc - m_line_start - 1);
				libbio_assert_neq(*sv.rbegin(), '\n');
				if (!delegate.handle_comment_line(*this, sv))
					return false;
			}
			
			action header {
				current_line_type = line_type::NONE;
				std::string_view const sv(1 + m_line_start, fpc - m_line_start - 1);
				libbio_assert_neq(*sv.rbegin(), '\n');
				if (!delegate.handle_identifier(*this, sv))
					return false;
			}
			
			action sequence {
				current_line_type = line_type::NONE;
				std::string_view const sv(m_line_start, fpc - m_line_start);
				libbio_assert_neq(*sv.rbegin(), '\n');
				if (!delegate.handle_sequence_line(*this, sv))
					return false;
			}
			
			action error {
				libbio_always_assert(m_fsm.p != m_fsm.pe);
				report_unexpected_character(fcurs);
			}
			
			action handle_eof {
				switch (current_line_type)
				{
					case line_type::NONE:
						return true;
					
					case line_type::HEADER:
						report_unexpected_eof(fcurs);
						return false;
					
					case line_type::SEQUENCE:
					{
						std::string_view const sv(m_line_start, fpc - m_line_start);
						return delegate.handle_sequence_line(*this, sv);
					}
					
					case line_type::COMMENT:
					{
						std::string_view const sv(1 + m_line_start, fpc - m_line_start - 1);
						return delegate.handle_comment_line(*this, sv);
					}
				}
			}
			
			comment_line	= (";" [^\n]*)		>comment_start	"\n" >comment;
			header_line		= (">" [^\n]*)		>header_start	"\n" >header;
			sequence_line	= ([A-Za-z*\-]*)	>sequence_start	"\n" >sequence;
			
			fasta_header	= comment_line* header_line;
			fasta_sequence	= (comment_line* sequence_line)+ comment_line*;
			
			fasta_records	= (fasta_header fasta_sequence)+;
			
			main :=
				(sequence_line | fasta_records)
				$eof(handle_eof)
				$err(error);
			
			write init;
			write exec;
		}%%
			
		return retval;
	}
}

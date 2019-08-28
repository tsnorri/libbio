/*
 * Copyright (c) 2019 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <libbio/fasta_reader.hh>


%% machine fasta_parser;
%% write data;

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
		
		m_fsm.p = handle.data();
		m_fsm.pe = m_fsm.p + handle.size();
		m_fsm.eof = m_fsm.pe;
		
		m_line_start = nullptr;
		m_lineno = 0;
		
		%%{
			variable p		m_fsm.p;
			variable pe		m_fsm.pe;
			variable eof	m_fsm.eof;
			
			action line_start {
				++m_lineno;
				m_line_start = fpc;
			}
			
			action comment {
				std::string_view const sv(1 + m_line_start, fpc - m_line_start - 1);
				if (!delegate.handle_comment_line(*this, sv))
					return false;
			}
			
			action header {
				std::string_view const sv(1 + m_line_start, fpc - m_line_start - 1);
				if (!delegate.handle_identifier(*this, sv))
					return false;
			}
			
			action sequence {
				std::string_view const sv(m_line_start, fpc - m_line_start);
				libbio_assert_neq(*sv.rbegin(), '\n');
				if (!delegate.handle_sequence_line(*this, sv))
					return false;
			}
			
			action error {
				if (m_fsm.p == m_fsm.pe)
					report_unexpected_eof(fcurs);
				else
					report_unexpected_character(fcurs);
			}
			
			comment_line	= (";" [^\n]*)		>line_start	"\n" >comment;
			header_line		= (">" [^\n]*)		>line_start	"\n" >header;
			sequence_line	= ([A-Za-z*\-]*)	>line_start	"\n" >sequence;
			
			fasta_header	= comment_line* header_line;
			fasta_sequence	= (comment_line* sequence_line)+ comment_line*;
			
			fasta_record	= (fasta_header fasta_sequence)+;
			
			main :=
				fasta_record
				$err(error)
				$eof{ retval = false; };
			
			write init;
			write exec;
		}%%
			
		return retval;
	}
}

/*
 * Copyright (c) 2019–2023 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <libbio/assert.hh>
#include <libbio/fasta_reader.hh>
#include <libbio/file_handle.hh>
#include <libbio/file_handling.hh>

namespace lb = libbio;


%% machine fasta_parser;
%% write data;

namespace
{
	class single_sequence_reader_delegate final : public lb::fasta_reader_delegate
	{
	public:
		typedef std::vector <char>	sequence_vector;
		
	protected:
		sequence_vector	&m_sequence;
		char const		*m_sequence_id{};
		bool			m_seen_non_seq{};
		bool			m_is_copying{};
		
	public:
		explicit single_sequence_reader_delegate(sequence_vector &sequence, char const *sequence_id):
			m_sequence(sequence),
			m_sequence_id(sequence_id)
		{
		}
		
		bool handle_comment_line(lb::fasta_reader_base &reader, std::string_view const &sv) override { m_seen_non_seq = true; return true; }
		
		bool handle_identifier(lb::fasta_reader_base &reader, std::string_view const &identifier, std::vector <std::string_view> const &extra) override
		{
			m_seen_non_seq = true;

			// FIXME: Check if the identifier has multiple fields.
			if (nullptr == m_sequence_id || identifier == m_sequence_id)
				m_is_copying = true;

			return true;
		}
		
		bool handle_sequence_chunk(lb::fasta_reader_base &reader, std::string_view const &sv, bool has_newline) override
		{
			if (!m_is_copying)
			{
				if (!m_seen_non_seq)
					m_is_copying = true;
				else
					return true;
			}
			
			std::copy(sv.begin(), sv.end(), std::back_inserter(m_sequence));
			return true;
		}
		
		bool handle_sequence_end(lb::fasta_reader_base &reader) override
		{
			return !m_is_copying;
		}
		
		bool did_find_matching_sequence() const { return m_is_copying; }
	};
}


namespace libbio
{
	// FIXME: Handle errors through the delegate.
	void fasta_reader::report_unexpected_character(char const *line_start, std::size_t const lineno, int const current_state) const
	{
		std::cerr << "Unexpected character 0x" << std::hex << +(*m_fsm.p) << std::dec << ' ';
		if (std::isprint(*m_fsm.p))
			std::cerr << "(‘" << (*m_fsm.p) << "’) ";
		std::cerr << "at " << lineno << ':' << (m_fsm.p - line_start)
			<< ", state " << current_state << '.' << std::endl;
		output_buffer_end(line_start);
		abort();
	}
	
	
	void fasta_reader::report_unexpected_eof(char const *line_start, std::size_t const lineno, int const current_state) const
	{
		std::cerr
			<< "Unexpected EOF at " << lineno << ':' << (m_fsm.p - line_start)
			<< ", state " << current_state << '.' << std::endl;
		output_buffer_end(line_start);
		abort();
	}
	
	
	void fasta_reader::output_buffer_end(char const *line_start) const
	{
		auto const start(m_fsm.p - line_start < 128 ? line_start : m_fsm.p - 128);
		auto const len(m_fsm.p - start);
		std::string_view buffer_end(start, len);
		std::cerr
		<< "** Last " << len << " charcters from the buffer:" << std::endl
		<< buffer_end << std::endl;
	}
	
	
	auto fasta_reader_base::parse(handle_type &handle, fasta_reader_delegate &delegate, std::size_t blocksize) -> parsing_status
	{
		if (0 == blocksize)
			blocksize = 16384; // Best guess.
		
		int cs(0);
		
		%%{
			action header_begin {
				++lineno;
				line_start = fpc;
				text_start = 1 + fpc;
				m_extra_fields.clear();
				seq_identifier_range.pos = 1;
			}
			
			action header_identifier_end {
				seq_identifier_range.end = fpc - line_start;
			}
			
			action header_extra_begin {
				text_start = fpc;
			}
			
			action header_extra_end {
				auto &extra(m_extra_fields.emplace_back(text_start - line_start, fpc - line_start));
			}
			
			action header_end {
				std::string_view const seq_identifier{line_start + seq_identifier_range.pos, line_start + seq_identifier_range.end};
				
				for (auto &extra : m_extra_fields)
				{
					auto const range(extra.rr);
					new (&extra.sv) std::string_view{line_start + range.pos, line_start + range.end};
				}
				
				auto const &extra_fields(reinterpret_cast <std::vector <std::string_view> const &>(m_extra_fields));
				if (!delegate.handle_identifier(*this, seq_identifier, extra_fields))
					return parsing_status::cancelled;
			}
			
			action comment_begin {
				++lineno;
				line_start = fpc;
				text_start = 1 + fpc;
			}
			
			action comment_end {
				if (!delegate.handle_comment_line(*this, std::string_view{text_start, fpc}))
					return parsing_status::cancelled;
			}
			
			action sequence_line_begin {
				++lineno;
				in_sequence = true;
				line_start = fpc;
				text_start = fpc;
			}
			
			action sequence_line_end {
				if (!delegate.handle_sequence_chunk(*this, std::string_view{text_start, fpc}, true))
					return parsing_status::cancelled;
				in_sequence = false;
			}
			
			action sequence_line_end_ {
				if (!delegate.handle_sequence_chunk(*this, std::string_view{text_start, fpc}, false))
					return parsing_status::cancelled;
				in_sequence = false;
			}
			
			action sequence_end {
				if (!delegate.handle_sequence_end(*this))
					return parsing_status::cancelled;
			}
			
			action header_eof {
				report_unexpected_eof(line_start, lineno, fcurs);
				return parsing_status::failure;
			}
			
			action main_eof {
				return parsing_status::success;
			}
			
			action error {
				libbio_always_assert(fpc != m_fsm.pe);
				report_unexpected_character(line_start, lineno, fcurs);
			}
			
			variable p		m_fsm.p;
			variable pe		m_fsm.pe;
			variable eof	m_fsm.eof;
			
			header_field			= (any - space)+;
			header_field_separator	= space - "\n";
			header_identifier_field	= header_field %header_identifier_end;
			header_extra_fields		= (header_field_separator+ (header_field >header_extra_begin %header_extra_end))*;
			header_line				= (">" header_identifier_field header_extra_fields "\n") >header_begin @header_end <eof(header_eof);
			
			# Allow missing final newline.
			
			sequence				= [A-Za-z*\-]+;
			sequence_line			= (sequence "\n") >sequence_line_begin @sequence_line_end;
			final_sequence_line		= sequence_line | (sequence >sequence_line_begin %sequence_line_end_);
			
			comment_line			= (";" [^\n]* "\n") >comment_begin @comment_end;
			final_comment_line		= comment_line | (";" [^\n]*) >comment_begin %comment_end;
			
			fasta_sequence_			= (comment_line* sequence_line)+ comment_line*;
			fasta_sequence			= fasta_sequence_ %sequence_end;
			final_fasta_sequence	= ((fasta_sequence_? final_sequence_line) | (fasta_sequence_ final_comment_line)) %sequence_end;
		
			fasta_records			= comment_line* (header_line fasta_sequence)* header_line final_fasta_sequence;
			sequence_only			= sequence_line* final_sequence_line %sequence_end;
			
			main := ("" | sequence_only | fasta_records) $eof(main_eof) $err(error);
			
			write init;
		}%%
		
		m_extra_fields.clear();
		m_buffer.clear();
		m_fsm = fsm{};
		
		bool in_sequence{};
		std::size_t lineno{};
		char const *line_start{m_buffer.data()};
		char const *text_start{line_start};
		range seq_identifier_range; // Relative to the current line.
		
		while (true)
		{
			// Save the previous contents of the buffer if needed.
			{
				auto const write_pos(m_buffer.size());
				auto const line_start_offset(line_start - m_buffer.data());
				auto const text_start_offset(text_start - m_buffer.data());
				m_buffer.resize(write_pos + blocksize);
				line_start = m_buffer.data() + line_start_offset;
				text_start = m_buffer.data() + text_start_offset;
				
				auto const count(handle.read(blocksize, m_buffer.data() + write_pos));
				m_buffer.resize(write_pos + count);
				
				m_fsm.p = m_buffer.data() + write_pos;
				m_fsm.pe = m_fsm.p + count;
				
				// std::vector <T>::data() not guaranteed to return nullptr even if empty() returns true.
				if (0 == count)
					m_fsm.eof = m_fsm.p;
			}
			
			%% write exec;
			
			// If we’re currently handling a sequence, pass it to the delegate.
			// Otherwise preserve the current line.
			if (in_sequence)
			{
				if (text_start < m_fsm.p && !delegate.handle_sequence_chunk(*this, std::string_view{text_start, m_fsm.p}, false))
					return parsing_status::cancelled;
				
				m_buffer.clear();
				text_start = m_buffer.data();
				line_start = m_buffer.data();
			}
			else
			{
				std::copy(line_start, m_fsm.pe, m_buffer.data());
				m_buffer.resize(m_fsm.pe - line_start); // We rely on the fact that this does not change the location of the buffer in memory.
				text_start -= line_start - m_buffer.data();
				line_start = m_buffer.data();
			}
		}
	}
	
	
	bool read_single_fasta_sequence(char const *fasta_path, std::vector <char> &seq, char const *seq_name)
	{
		file_handle handle{lb::open_file_for_reading(fasta_path)};
		seq.clear();
		
		single_sequence_reader_delegate delegate(seq, seq_name);
		
		fasta_reader reader;
		auto const res(reader.parse(handle, delegate));
		
		return fasta_reader::parsing_status::success == res || fasta_reader::parsing_status::cancelled == res;
	}
}

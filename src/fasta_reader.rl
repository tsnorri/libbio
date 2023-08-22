/*
 * Copyright (c) 2019 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <libbio/fasta_reader.hh>

namespace lb = libbio;


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
	
	
	class single_sequence_reader_delegate final : public lb::fasta_reader_delegate
	{
	public:
		typedef std::vector <char>	sequence_vector;
		
	protected:
		sequence_vector	&m_sequence;
		char const		*m_sequence_id{};
		bool			m_is_copying{};
		
	public:
		explicit single_sequence_reader_delegate(sequence_vector &sequence, char const *sequence_id):
			m_sequence(sequence),
			m_sequence_id(sequence_id)
		{
		}
		
		bool handle_comment_line(lb::fasta_reader &reader, std::string_view const &sv) override { return true; }
		
		bool handle_identifier(lb::fasta_reader &reader, std::string_view const &identifier, std::string_view const &extra) override
		{
			// FIXME: Check if the identifier has multiple fields.
			if (m_is_copying)
				return false;
			
			if (nullptr == m_sequence_id || identifier == m_sequence_id)
				m_is_copying = true;

			return true;
		}
		
		bool handle_sequence_line(lb::fasta_reader &reader, std::string_view const &sv) override
		{
			if (!m_is_copying)
				return true;
			
			std::copy(sv.begin(), sv.end(), std::back_inserter(m_sequence));
			return true;
		}
		
		bool did_find_matching_sequence() const { return m_is_copying; }
	};
}


namespace libbio
{
	// FIXME: Handle errors through the delegate.
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
	
	
	auto fasta_reader::parse(handle_type &handle, fasta_reader_delegate &delegate) -> parsing_status
	{
		int cs(0);
		line_type current_line_type(line_type::NONE);
		char const *header_identifier_end{nullptr};
		char const *header_extra_start{nullptr};
		
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

			action header_identifier {
				header_identifier_end = fpc;
			}

			action header_extra {
				header_extra_start = fpc;
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
					return parsing_status::cancelled;
			}
			
			action header {
				current_line_type = line_type::NONE;

				std::string_view const identifier(1 + m_line_start, header_identifier_end);
				libbio_assert_neq(*identifier.rbegin(), '\n');

				std::string_view extra{};
				if (header_extra_start)
				{
					extra = std::string_view(header_extra_start, fpc - 1);
					libbio_assert_neq(*extra.rbegin(), '\n');
				}

				header_identifier_end = nullptr;
				header_extra_start = nullptr;

				if (!delegate.handle_identifier(*this, identifier, extra))
					return parsing_status::cancelled;
			}
			
			action sequence {
				current_line_type = line_type::NONE;
				std::string_view const sv(m_line_start, fpc - m_line_start);
				libbio_assert_neq(*sv.rbegin(), '\n');
				if (!delegate.handle_sequence_line(*this, sv))
					return parsing_status::cancelled;
			}
			
			action error {
				libbio_always_assert(m_fsm.p != m_fsm.pe);
				report_unexpected_character(fcurs);
			}
			
			action handle_eof {
				switch (current_line_type)
				{
					case line_type::NONE:
						return parsing_status::success;
					
					case line_type::HEADER:
						report_unexpected_eof(fcurs);
						return parsing_status::failure;
					
					case line_type::SEQUENCE:
					{
						std::string_view const sv(m_line_start, fpc - m_line_start);
						return delegate.handle_sequence_line(*this, sv) ? parsing_status::success : parsing_status::cancelled;
					}
					
					case line_type::COMMENT:
					{
						std::string_view const sv(1 + m_line_start, fpc - m_line_start - 1);
						return delegate.handle_comment_line(*this, sv) ? parsing_status::success : parsing_status::cancelled;
					}
				}
			}

			header_field			= (any - space)+;
			header_field_separator	= space - "\n";
			
			comment_line	= (";" [^\n]*)			>comment_start	"\n" >comment;
			header_start	= (">" header_field) 	>header_start %header_identifier;
			header_end		= (header_field_separator+ (header_field >header_extra))? (header_field_separator+ header_field)* "\n" >header;
			header_line		= header_start header_end;
			sequence_line	= ([A-Za-z*\-]*)											>sequence_start	"\n" >sequence;
			
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
			
		return parsing_status::success;
	}
	
	
	bool read_single_fasta_sequence(char const *fasta_path, std::vector <char> &seq, char const *seq_name)
	{
		lb::mmap_handle <char> handle;
		handle.open(fasta_path);
		
		seq.clear();
		seq.reserve(handle.size());
		
		single_sequence_reader_delegate delegate(seq, seq_name);
		
		fasta_reader reader;
		auto const res(reader.parse(handle, delegate));
		
		return fasta_reader::parsing_status::success == res || fasta_reader::parsing_status::cancelled == res;
	}
}

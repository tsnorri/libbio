/*
 * Copyright (c) 2019–2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <libbio/assert.hh>
#include <libbio/fasta_reader.hh>
#include <libbio/file_handle.hh>
#include <libbio/file_handling.hh>
#include <string_view>
#include <vector>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"


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
		bool			m_is_copying{};

	public:
		explicit single_sequence_reader_delegate(sequence_vector &sequence, char const *sequence_id):
			m_sequence(sequence),
			m_sequence_id(sequence_id)
		{
		}

		bool handle_identifier(lb::fasta_reader_base &reader, std::string_view identifier, std::span <std::string_view const> extra) override
		{
			// FIXME: Check if the identifier has multiple fields.
			if (nullptr == m_sequence_id || identifier == m_sequence_id)
				m_is_copying = true;

			return true;
		}

		bool handle_sequence_chunk(lb::fasta_reader_base &reader, std::string_view sv, bool has_newline) override
		{
			if (!m_is_copying)
				return true;

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
	void fasta_reader::report_unexpected_character(int current_state) const
	{
		std::cerr << "Unexpected character 0x" << std::hex << +(*m_fsm.p) << std::dec << ' ';
		if (std::isprint(*m_fsm.p))
			std::cerr << "(‘" << (*m_fsm.p) << "’) ";
		std::cerr << "at " << m_fsm.lineno << ':' << (m_fsm.p - m_fsm.line_start)
			<< ", state " << current_state << '.' << std::endl;
		output_buffer_end();
		std::abort();
	}


	void fasta_reader::report_unexpected_eof(int current_state) const
	{
		std::cerr
			<< "Unexpected EOF at " << m_fsm.lineno << ':' << (m_fsm.p - m_fsm.line_start)
			<< ", state " << current_state << '.' << std::endl;
		output_buffer_end();
		std::abort();
	}


	void fasta_reader::output_buffer_end() const
	{
		auto const start(m_fsm.p - m_fsm.line_start < 128 ? m_fsm.line_start : m_fsm.p - 128);
		auto const len(m_fsm.p - start);
		std::string_view buffer_end(start, len);
		std::cerr
			<< "** Last " << len << " charcters from the buffer:" << std::endl
			<< buffer_end << std::endl;
	}


	void fasta_reader_base::prepare()
	{
		m_fsm = fsm{m_buffer.data()};
		m_extra_fields.clear();
		m_buffer.clear();
		%% write init;
	}


	auto fasta_reader_base::parse(handle_type &handle, std::size_t blocksize) -> parsing_status
	{
		prepare();
		return parse_(handle, blocksize);
	}


	auto fasta_reader_base::parse_(handle_type &handle, std::size_t blocksize) -> parsing_status
	{
		if (0 == blocksize)
			blocksize = 16384; // Best guess.

		bool should_stop{};

		%%{
			action handle_newline {
				++m_fsm.lineno;
			}

			# If fbreak is used in a final state action, the subsequent start state action
			# will not be executed. (As of version 6.10, Ragel’s manual § 3.4 states that
			# this happens only with to-state actions.) To resolve the issue, we update
			# should_stop and check its value in the following start state action.
			action header {
				m_fsm.line_start = fpc;
				m_fsm.text_start = 1 + fpc;
				m_fsm.seq_identifier_range.pos = 1;

				if (should_stop)
				{
					fhold;
					fbreak;
				}
			}

			action header_identifier_end {
				m_fsm.seq_identifier_range.end = fpc - m_fsm.line_start;
			}

			action header_extra {
				m_fsm.text_start = fpc;
			}

			action header_extra_end {
				auto &extra(m_extra_fields.emplace_back(m_fsm.text_start - m_fsm.line_start, fpc - m_fsm.line_start));
			}

			action header_end {
				std::string_view const seq_identifier{m_fsm.line_start + m_fsm.seq_identifier_range.pos, m_fsm.line_start + m_fsm.seq_identifier_range.end};

				for (auto &extra : m_extra_fields)
				{
					auto const range(extra.rr);
					new (&extra.sv) std::string_view{m_fsm.line_start + range.pos, m_fsm.line_start + range.end};
				}

				std::span const extra_fields{reinterpret_cast <std::string_view *>(m_extra_fields.data()), m_extra_fields.size()};
				should_stop = !m_delegate->handle_identifier(*this, seq_identifier, extra_fields);
				m_extra_fields.clear();
			}

			# See action header.
			action sequence_line {
				m_fsm.in_sequence = true;
				m_fsm.line_start = fpc;
				m_fsm.text_start = fpc;

				if (should_stop)
				{
					fhold;
					fbreak;
				}
			}

			action sequence_line_end {
				m_fsm.in_sequence = false;
				should_stop = !m_delegate->handle_sequence_chunk(*this, std::string_view{m_fsm.text_start, fpc}, true);
			}

			action sequence_line_end_no_eol {
				m_fsm.in_sequence = false;
				should_stop = !m_delegate->handle_sequence_chunk(*this, std::string_view{m_fsm.text_start, fpc}, false);
			}

			action sequence_end {
				should_stop = !m_delegate->handle_sequence_end(*this);
			}

			action unexpected_eof {
				report_unexpected_eof(fcurs);
				return parsing_status::failure; // No need to preserve state.
			}

			action main_eof {
				return parsing_status::success; // No need to preserve state.
			}

			action error {
				libbio_always_assert(fpc != m_fsm.pe);
				report_unexpected_character(fcurs);
			}

			access			m_fsm.;
			variable p		m_fsm.p;
			variable pe		m_fsm.pe;
			variable eof	m_fsm.eof;

			nl						= "\n" @handle_newline;

			header_field			= (any - space)+;
			header_field_separator	= space - "\n";
			header_identifier_field	= header_field %header_identifier_end;
			header_extra_fields		= (header_field_separator+ (header_field >header_extra %header_extra_end))*;
			header_line				= ([>;] header_identifier_field header_extra_fields nl) >header @header_end <eof(unexpected_eof);

			# Allow missing final newline.

			sequence_line			= ([A-Za-z*\-]+ nl) >sequence_line @sequence_line_end;
			final_sequence_line_	= ([A-Za-z*\-]+) >sequence_line %eof(sequence_line_end_no_eol);
			final_sequence_line		= sequence_line | final_sequence_line_;

			fasta_sequence			= (sequence_line+) %sequence_end;
			final_fasta_sequence	= (sequence_line* final_sequence_line) %sequence_end;

			fasta_records			= (header_line fasta_sequence)* header_line final_fasta_sequence;
			sequence_only			= final_fasta_sequence;

			main := ("" | sequence_only | fasta_records) $eof(main_eof) $err(error);
		}%%

		while (0 != m_fsm.cs)
		{
			// Save the previous contents of the buffer if needed.
			if (m_fsm.p == m_fsm.pe)
			{
				auto const write_pos(m_buffer.size());
				auto const line_start_offset(m_fsm.line_start - m_buffer.data());
				auto const text_start_offset(m_fsm.text_start - m_buffer.data());
				m_buffer.resize(write_pos + blocksize);
				m_fsm.line_start = m_buffer.data() + line_start_offset;
				m_fsm.text_start = m_buffer.data() + text_start_offset;

				auto const count(handle.read(blocksize, m_buffer.data() + write_pos));
				m_buffer.resize(write_pos + count);

				m_fsm.p = m_buffer.data() + write_pos;
				m_fsm.pe = m_fsm.p + count;

				// std::vector <T>::data() not guaranteed to return nullptr even if empty() returns true.
				if (0 == count)
					m_fsm.eof = m_fsm.p;
			}

			%% write exec;

			if (should_stop)
				return parsing_status::cancelled;

			// If we’re currently handling a sequence, pass it to the delegate.
			// Otherwise preserve the current line.
			if (m_fsm.in_sequence)
			{
				if (m_fsm.text_start < m_fsm.p && !m_delegate->handle_sequence_chunk(*this, std::string_view{m_fsm.text_start, m_fsm.p}, false))
					return parsing_status::cancelled;

				m_buffer.clear();
				m_fsm.line_start = m_buffer.data();
				m_fsm.text_start = m_buffer.data();
				m_fsm.p = m_buffer.data();
				m_fsm.pe = m_fsm.p;
			}
			else if (m_buffer.data() < m_fsm.line_start) // Avoid UB in std::copy and make calculating diff possible.
			{
				auto const diff{m_fsm.line_start - m_buffer.data()};
				std::copy(m_fsm.line_start, m_fsm.pe, m_buffer.data());
				m_buffer.resize(m_fsm.pe - m_fsm.line_start); // We rely on the fact that this does not change the location of the buffer in memory.
				m_fsm.text_start -= m_fsm.line_start - m_buffer.data();
				m_fsm.line_start = m_buffer.data();
				m_fsm.p -= diff;
				m_fsm.pe -= diff;
			}
		}

		return parsing_status::success;
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

#pragma GCC diagnostic pop

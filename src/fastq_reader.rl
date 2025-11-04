/*
 * Copyright (c) 2025 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <algorithm>
#include <iostream>
#include <libbio/assert.hh>
#include <libbio/fastq_reader.hh>
#include <string_view>
#include <vector>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"


namespace lb = libbio;


%% machine fastq_parser;
%% write data;


namespace {

	enum class handling_state
	{
		none,
		in_sequence,
		in_quality
	};
}


namespace libbio {

	// FIXME: Handle errors through the delegate?
	void fastq_reader::report_unexpected_character(char const *line_start, std::size_t const lineno, int const current_state) const
	{
		std::cerr << "Unexpected character 0x" << std::hex << +(*m_fsm.p) << std::dec << ' ';
		if (std::isprint(*m_fsm.p))
			std::cerr << "(‘" << (*m_fsm.p) << "’) ";
		std::cerr << "at " << lineno << ':' << (m_fsm.p - line_start)
			<< ", state " << current_state << '.' << std::endl;
		output_buffer_end(line_start);
		std::abort();
	}


	void fastq_reader::report_unexpected_eof(char const *line_start, std::size_t const lineno, int const current_state) const
	{
		std::cerr
			<< "Unexpected EOF at " << lineno << ':' << (m_fsm.p - line_start)
			<< ", state " << current_state << '.' << std::endl;
		output_buffer_end(line_start);
		std::abort();
	}


	void fastq_reader::report_length_mismatch(char const *line_start, std::size_t const lineno, int const current_state) const
	{
		std::cerr
			<< "Sequence length mismatch at " << lineno << ':' << (m_fsm.p - line_start)
			<< ", state " << current_state << '.' << std::endl;
		output_buffer_end(line_start);
		std::abort();
	}


	void fastq_reader::output_buffer_end(char const *line_start) const
	{
		auto const start(m_fsm.p - line_start < 128 ? line_start : m_fsm.p - 128);
		auto const len(m_fsm.p - start);
		std::string_view buffer_end(start, len);
		std::cerr
		<< "** Last " << len << " charcters from the buffer:" << std::endl
		<< buffer_end << std::endl;
	}


	auto fastq_reader_base::parse(handle_type &handle, fastq_reader_delegate &delegate, std::size_t blocksize) -> parsing_status
	{
		if (0U == blocksize)
			blocksize = 16384U; // Best guess.

		int cs{};

		%%{
			action handle_newline {
				++lineno;
			}

			action identifier_line {
				line_start = fpc;
				text_start = fpc;
				sequence_length = 0;
				quality_length = 0;
			}

			action identifier_end {
				if (!delegate.handle_identifier(*this, std::string_view{line_start + 1U, fpc}))
					return parsing_status::cancelled;
			}

			action sequence_line {
				state = handling_state::in_sequence;
				line_start = fpc;
				text_start = fpc;
			}

			action sequence_line_end {
				{
					std::string_view sv{text_start, fpc};
					if (!delegate.handle_sequence_chunk(*this, sv, true))
						return parsing_status::cancelled;
					state = handling_state::none;
					sequence_length += sv.size();
				}
			}

			action sequence_end {
				if (!delegate.handle_sequence_end(*this))
					return parsing_status::cancelled;
			}

			action qual_line {
				state = handling_state::in_quality;
				line_start = fpc;
				text_start = fpc;
			}

			action qual_line_end {
				{
					std::string_view sv{text_start, fpc};
					if (!delegate.handle_quality_chunk(*this, sv, true))
						return parsing_status::cancelled;

					state = handling_state::none;

					quality_length += sv.size();
					if (sequence_length == quality_length)
					{
						if (!delegate.handle_quality_end(*this))
							return parsing_status::cancelled;
						fnext main;
					}
					else if (sequence_length < quality_length)
					{
						report_length_mismatch(line_start, lineno, fcurs);
					}
				}
			}

			action qual_line_end_no_nl {
				if (!delegate.handle_quality_chunk(*this, std::string_view{text_start, fpc + 1}, true))
					return parsing_status::cancelled;
				state = handling_state::none;
			}

			action record_head_end {
				fnext qual_lines;
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

			nl					= "\n"										@handle_newline;

			seqname_			= [A-Za-z0-9_.:\-]+;
			seqname				= ("@" seqname_ nl)							>identifier_line @identifier_end;

			sequence_			= [A-Za-z\.~]+;
			sequence_line		= (sequence_ nl)							>sequence_line @sequence_line_end;
			sequence			= (sequence_line+)							%sequence_end;

			# @ can appear in the quality string in addition to being the block starting character.
			# Hence the block end cannot be determined only by looking for the next @. We solve the
			# issue by counting characters in the sequence and quality strings.

			qual				= [!-~]+;
			qual_line			= (qual nl)									>qual_line @qual_line_end;
			last_qual_line		= qual										>qual_line <eof(qual_line_end_no_nl);
			qual_lines			:= qual_line* (qual_line | last_qual_line)	$err(error);

			fastq_record_head	= (seqname sequence "+" seqname_? nl)		@record_head_end;

			main := fastq_record_head >eof(main_eof) $err(error);

			write init;
		}%%

		m_fsm = fsm{};
		handling_state state{handling_state::none};
		std::size_t lineno{1U};
		char const *line_start{m_buffer.data()};
		char const *text_start{line_start};
		std::size_t sequence_length{};
		std::size_t quality_length{};

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

			// If we’re currently handling a sequence or a quality string, pass it to the delegate.
			// Otherwise preserve the current line.
			auto const clear_buffer([&]{
				m_buffer.clear();
				text_start = m_buffer.data();
				line_start = m_buffer.data();
			});

			switch (state)
			{
				case handling_state::in_sequence:
				{
					std::string_view sv{text_start, m_fsm.p};
					if (text_start < m_fsm.p && !delegate.handle_sequence_chunk(*this, sv, false))
						return parsing_status::cancelled;
					sequence_length += sv.size();
					clear_buffer();
					break;
				}

				case handling_state::in_quality:
				{
					std::string_view sv{text_start, m_fsm.p};
					quality_length += sv.size();
					if (sequence_length < quality_length)
						report_length_mismatch(line_start, lineno, cs);
					if (text_start < m_fsm.p && !delegate.handle_quality_chunk(*this, sv, false))
						return parsing_status::cancelled;
					clear_buffer();
					break;
				}

				case handling_state::none:
				{
					std::copy(line_start, m_fsm.pe, m_buffer.data());
					m_buffer.resize(m_fsm.pe - line_start); // We rely on the fact that this does not change the location of the buffer in memory.
					text_start -= line_start - m_buffer.data();
					line_start = m_buffer.data();
				}
			}
		}
	}
}

#pragma GCC diagnostic pop

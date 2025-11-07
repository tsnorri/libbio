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


namespace libbio {

	// FIXME: Handle errors through the delegate?
	void fastq_reader::report_unexpected_character(int const current_state) const
	{
		std::cerr << "Unexpected character 0x" << std::hex << +(*m_fsm.p) << std::dec << ' ';
		if (std::isprint(*m_fsm.p))
			std::cerr << "(‘" << (*m_fsm.p) << "’) ";
		std::cerr << "at " << m_fsm.lineno << ':' << (m_fsm.p - m_fsm.line_start)
			<< ", state " << current_state << '.' << std::endl;
		output_buffer_end();
		std::abort();
	}


	void fastq_reader::report_unexpected_eof(int const current_state) const
	{
		std::cerr
			<< "Unexpected EOF at " << m_fsm.lineno << ':' << (m_fsm.p - m_fsm.line_start)
			<< ", state " << current_state << '.' << std::endl;
		output_buffer_end();
		std::abort();
	}


	void fastq_reader::report_length_mismatch(int const current_state) const
	{
		std::cerr
			<< "Sequence length mismatch at " << m_fsm.lineno << ':' << (m_fsm.p - m_fsm.line_start)
			<< ", state " << current_state << '.' << std::endl;
		output_buffer_end();
		std::abort();
	}


	void fastq_reader::output_buffer_end() const
	{
		auto const start(m_fsm.p - m_fsm.line_start < 128 ? m_fsm.line_start : m_fsm.p - 128);
		auto const len(m_fsm.p - start);
		std::string_view buffer_end(start, len);
		std::cerr
			<< "** Last " << len << " charcters from the buffer:" << std::endl
			<< buffer_end << std::endl;
	}


	void fastq_reader_base::prepare()
	{
		m_fsm = fsm{m_buffer.data()};
		%% write init;
	}


	auto fastq_reader_base::parse(handle_type &handle, fastq_reader_delegate &delegate, std::size_t blocksize) -> parsing_status
	{
		prepare();
		return parse_(handle, delegate, blocksize);
	}


	auto fastq_reader_base::parse_(handle_type &handle, fastq_reader_delegate &delegate, std::size_t blocksize) -> parsing_status
	{
		if (0U == blocksize)
			blocksize = 16384U; // Best guess.

		%%{
			action handle_newline {
				++m_fsm.lineno;
			}

			action identifier_line {
				m_fsm.line_start = fpc;
				m_fsm.text_start = fpc;
				m_fsm.sequence_length = 0;
				m_fsm.quality_length = 0;
			}

			action identifier_end {
				if (!delegate.handle_identifier(*this, std::string_view{m_fsm.line_start + 1U, fpc}))
				{
					++fpc;
					return parsing_status::cancelled;
				}
			}

			action sequence_line {
				m_fsm.state = handling_state::in_sequence;
				m_fsm.line_start = fpc;
				m_fsm.text_start = fpc;
			}

			action sequence_line_end {
				{
					std::string_view sv{m_fsm.text_start, fpc};
					m_fsm.state = handling_state::none;
					m_fsm.sequence_length += sv.size();
					if (!delegate.handle_sequence_chunk(*this, sv, true))
					{
						++fpc;
						return parsing_status::cancelled;
					}
				}
			}

			action sequence_end {
				if (!delegate.handle_sequence_end(*this))
				{
					++fpc;
					return parsing_status::cancelled;
				}
			}

			action qual_line {
				m_fsm.state = handling_state::in_quality;
				m_fsm.line_start = fpc;
				m_fsm.text_start = fpc;
			}

			action qual_line_end {
				{
					std::string_view sv{m_fsm.text_start, fpc};
					m_fsm.state = handling_state::none;
					m_fsm.quality_length += sv.size();

					if (!delegate.handle_quality_chunk(*this, sv, true))
					{
						++fpc;
						return parsing_status::cancelled;
					}

					if (m_fsm.sequence_length == m_fsm.quality_length)
					{
						fnext main;
						if (!delegate.handle_quality_end(*this))
						{
							++fpc;
							return parsing_status::cancelled;
						}
					}
					else if (m_fsm.sequence_length < m_fsm.quality_length)
					{
						report_length_mismatch(fcurs);
					}
				}
			}

			action record_head_end {
				fnext qual_lines;
			}

			action unexpected_eof {
				report_unexpected_eof(fcurs);
				return parsing_status::failure;
			}

			action main_eof {
				return parsing_status::success;
			}

			action error {
				libbio_always_assert(fpc != m_fsm.pe);
				report_unexpected_character(fcurs);
				return parsing_status::failure;
			}

			access			m_fsm.;
			variable p		m_fsm.p;
			variable pe		m_fsm.pe;
			variable eof	m_fsm.eof;

			nl					= "\n"											@handle_newline;

			seqname_			= [A-Za-z0-9_.:\-]+;
			seqname				= ("@" seqname_ nl)								>identifier_line @identifier_end <eof(unexpected_eof);

			sequence_			= [A-Za-z\.~]+;
			sequence_line		= (sequence_ nl)								>sequence_line @sequence_line_end;
			sequence			= (sequence_line+)								%sequence_end $eof(unexpected_eof);

			# @ can appear in the quality string in addition to being the block starting character.
			# Hence the block end cannot be determined only by looking for the next @. We solve the
			# issue by counting characters in the sequence and quality strings.

			qual				= [!-~]+;
			qual_line			= (qual nl)										>qual_line @qual_line_end;
			last_qual_line		= qual											>qual_line %eof(qual_line_end);
			qual_lines			:= (qual_line* (qual_line | last_qual_line))	$err(error) >eof(unexpected_eof);

			fastq_record_head	= (seqname sequence "+" seqname_? nl)			@record_head_end <eof(unexpected_eof);

			main := fastq_record_head >eof(main_eof) $err(error);
		}%%

		while (true)
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

			// If we’re currently handling a sequence or a quality string, pass it to the delegate.
			// Otherwise preserve the current line.
			auto const clear_buffer([&]{
				m_buffer.clear();
				m_fsm.text_start = m_buffer.data();
				m_fsm.line_start = m_buffer.data();
			});

			switch (m_fsm.state)
			{
				case handling_state::in_sequence:
				{
					std::string_view sv{m_fsm.text_start, m_fsm.p};
					m_fsm.sequence_length += sv.size();
					if (m_fsm.text_start < m_fsm.p && !delegate.handle_sequence_chunk(*this, sv, false))
						return parsing_status::cancelled;
					clear_buffer();
					break;
				}

				case handling_state::in_quality:
				{
					std::string_view sv{m_fsm.text_start, m_fsm.p};
					m_fsm.quality_length += sv.size();
					if (m_fsm.sequence_length < m_fsm.quality_length)
						report_length_mismatch(m_fsm.cs);
					if (m_fsm.text_start < m_fsm.p && !delegate.handle_quality_chunk(*this, sv, false))
						return parsing_status::cancelled;
					clear_buffer();
					break;
				}

				case handling_state::none:
				{
					std::copy(m_fsm.line_start, m_fsm.pe, m_buffer.data());
					m_buffer.resize(m_fsm.pe - m_fsm.line_start); // We rely on the fact that this does not change the location of the buffer in memory.
					m_fsm.text_start -= m_fsm.line_start - m_buffer.data();
					m_fsm.line_start = m_buffer.data();
				}
			}
		}
	}
}

#pragma GCC diagnostic pop

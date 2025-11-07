/*
 * Copyright (c) 2025 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_FASTQ_READER_HH
#define LIBBIO_FASTQ_READER_HH

#include <cstddef>
#include <libbio/file_handle.hh>
#include <string_view>
#include <vector>


namespace libbio {

	class fastq_reader_base;

	struct fastq_reader_delegate
	{
		virtual ~fastq_reader_delegate() {}
		virtual bool handle_identifier(fastq_reader_base &reader, std::string_view const &sv) = 0;
		virtual bool handle_sequence_chunk(fastq_reader_base &reader, std::string_view const &sv, bool has_newline) = 0; // The string view does not have the newline character.
		virtual bool handle_sequence_end(fastq_reader_base &reader) = 0;
		virtual bool handle_quality_chunk(fastq_reader_base &reader, std::string_view const &sv, bool has_newline) = 0;
		virtual bool handle_quality_end(fastq_reader_base &reader) = 0;
	};


	// Suitable for reading whole files, i.e. no random access.
	class fastq_reader_base
	{
	public:
		typedef reading_handle	handle_type;

		enum class parsing_status {
			success,
			failure,
			cancelled
		};

		enum class handling_state
		{
			none,
			in_sequence,
			in_quality
		};

		struct fsm
		{
			char const		*p{};
			char const		*pe{};
			char const		*eof{};

			char const		*line_start{};
			char const		*text_start{};

			std::size_t		lineno{1U};
			std::size_t		sequence_length{};
			std::size_t		quality_length{};

			int				cs{};
			handling_state	state{handling_state::none};

			constexpr fsm() = default;

			explicit fsm(char const *line_start_):
				line_start{line_start_},
				text_start{line_start_}
			{
			}
		};

	protected:
		std::vector <char>						m_buffer;
		fsm										m_fsm;

	protected:
		virtual void report_unexpected_character(int current_state) const = 0;
		virtual void report_unexpected_eof(int current_state) const = 0;
		virtual void report_length_mismatch(int current_state) const = 0;

	public:
		virtual ~fastq_reader_base() {}

		parsing_status parse(handle_type &handle, fastq_reader_delegate &delegate, std::size_t blocksize);
		parsing_status parse(handle_type &handle, fastq_reader_delegate &delegate) { return parse(handle, delegate, handle.io_op_blocksize()); }

		// Prepare manually.
		void prepare();
		parsing_status parse_(handle_type &handle, fastq_reader_delegate &delegate, std::size_t blocksize);
		parsing_status parse_(handle_type &handle, fastq_reader_delegate &delegate) { return parse_(handle, delegate, handle.io_op_blocksize()); }
	};


	class fastq_reader final : public fastq_reader_base
	{
	private:
		[[noreturn]] void report_unexpected_character(int current_state) const;
		[[noreturn]] void report_unexpected_eof(int current_state) const;
		[[noreturn]] void report_length_mismatch(int current_state) const;
		void output_buffer_end() const;
	};
}

#endif

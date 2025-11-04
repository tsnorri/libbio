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

		struct fsm
		{
			char const	*p{nullptr};
			char const	*pe{nullptr};
			char const	*eof{nullptr};
		};

	protected:
		std::vector <char>						m_buffer;
		fsm										m_fsm;

	protected:
		virtual void report_unexpected_character(char const *line_start, std::size_t const lineno, int const current_state) const = 0;
		virtual void report_unexpected_eof(char const *line_start, std::size_t const lineno, int const current_state) const = 0;
		virtual void report_length_mismatch(char const *line_start, std::size_t const lineno, int const current_state) const = 0;

	public:
		virtual ~fastq_reader_base() {}
		parsing_status parse(handle_type &handle, fastq_reader_delegate &delegate, std::size_t blocksize);
		parsing_status parse(handle_type &handle, fastq_reader_delegate &delegate) { return parse(handle, delegate, handle.io_op_blocksize()); }
	};


	class fastq_reader final : public fastq_reader_base
	{
	private:
		[[noreturn]] void report_unexpected_character(char const *line_start, std::size_t const lineno, int const current_state) const;
		[[noreturn]] void report_unexpected_eof(char const *line_start, std::size_t const lineno, int const current_state) const;
		[[noreturn]] void report_length_mismatch(char const *line_start, std::size_t const lineno, int const current_state) const;
		void output_buffer_end(char const *line_start) const;
	};
}

#endif

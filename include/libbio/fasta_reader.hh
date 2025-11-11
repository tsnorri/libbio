/*
 * Copyright (c) 2016-2025 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_FASTA_READER_HH
#define LIBBIO_FASTA_READER_HH

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <libbio/file_handle.hh>
#include <libbio/sequence_reader.hh>
#include <libbio/utility/variable_guard.hh>
#include <span>
#include <string_view>
#include <type_traits>
#include <vector>


namespace libbio {

	class fasta_reader_base;


	struct fasta_reader_delegate
	{
		virtual ~fasta_reader_delegate() {}
		virtual bool handle_identifier(fasta_reader_base &reader, std::string_view sv, std::span <std::string_view const> additional_info) = 0;
		virtual bool handle_sequence_chunk(fasta_reader_base &reader, std::string_view sv, bool has_newline) = 0; // The string view does not have the newline character.
		virtual bool handle_sequence_end(fasta_reader_base &reader) = 0;
	};


	// Suitable for reading whole files, i.e. no random access.
	class fasta_reader_base : public sequence_reader
	{
	protected:
		struct range
		{
			std::size_t pos{};
			std::size_t end{};
		};

		struct fsm
		{
			char const	*p{};
			char const	*pe{};
			char const	*eof{};

			char const *line_start{};
			char const *text_start{};

			range seq_identifier_range; // Relative to the current line.

			std::size_t lineno{1U};
			int cs{};
			bool in_sequence{};

			constexpr fsm() = default;

			explicit fsm(char const *text_start_):
				text_start{text_start_}
			{
			}
		};

		// Try to save a bit of memory, as well as memory accesses.
		union string_view_placeholder
		{
			range				rr{};
			std::string_view	sv;

			string_view_placeholder(std::size_t pos, std::size_t end):
				rr{pos, end}
			{
			}
		};

		static_assert(std::is_trivially_copyable_v <std::string_view>);
		static_assert(std::is_trivially_copyable_v <string_view_placeholder>);
		static_assert(sizeof(string_view_placeholder) == sizeof(std::string_view));

	protected:
		std::vector <string_view_placeholder>	m_extra_fields;
		std::vector <char>						m_buffer;
		fsm										m_fsm;
		fasta_reader_delegate					*m_delegate{};

	protected:
		virtual void report_unexpected_character(int current_state) const = 0;
		virtual void report_unexpected_eof(int current_state) const = 0;

	public:
		virtual ~fasta_reader_base() {}

		void set_delegate(fasta_reader_delegate &delegate) { m_delegate = &delegate; }

		parsing_status parse(handle_type &handle, std::size_t blocksize) final;
		parsing_status parse(handle_type &handle, fasta_reader_delegate &delegate, std::size_t blocksize) { variable_guard gg{m_delegate, delegate}; return parse(handle, blocksize);; }
		parsing_status parse(handle_type &handle, fasta_reader_delegate &delegate) { return parse(handle, delegate, handle.io_op_blocksize()); }

		// Prepare manually.
		void prepare() final;
		parsing_status parse_(handle_type &handle, std::size_t blocksize) final;
		parsing_status parse_(handle_type &handle, fasta_reader_delegate &delegate, std::size_t blocksize) { variable_guard gg{m_delegate, delegate}; return parse_(handle, blocksize); }
		parsing_status parse_(handle_type &handle, fasta_reader_delegate &delegate) { return parse_(handle, delegate, handle.io_op_blocksize()); }

		std::uint64_t line_number() const final { return m_fsm.lineno; }
	};


	class fasta_reader final : public fasta_reader_base
	{
	private:
		[[noreturn]] void report_unexpected_character(int current_state) const override;
		[[noreturn]] void report_unexpected_eof(int current_state) const override;
		void output_buffer_end() const;
	};


	bool read_single_fasta_sequence(char const *fasta_path, std::vector <char> &seq, char const *seq_name = nullptr);
	inline bool read_single_fasta_sequence(std::filesystem::path const &fasta_path, std::vector <char> &seq, char const *seq_name = nullptr) { return read_single_fasta_sequence(fasta_path.c_str(), seq, seq_name); }
}

#endif

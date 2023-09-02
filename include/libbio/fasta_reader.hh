/*
 * Copyright (c) 2016-2019 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_FASTA_READER_HH
#define LIBBIO_FASTA_READER_HH

#include <iostream>
#include <libbio/cxxcompat.hh>
#include <libbio/file_handle.hh>
#include <string_view>
#include <vector>


namespace libbio { namespace detail {
}}


namespace libbio {
	
	class fasta_reader_base;
	
	
	struct fasta_reader_delegate
	{
		virtual ~fasta_reader_delegate() {}
		virtual bool handle_comment_line(fasta_reader_base &reader, std::string_view const &sv) = 0;
		virtual bool handle_identifier(fasta_reader_base &reader, std::string_view const &sv, std::vector <std::string_view> const &additional_info) = 0;
		virtual bool handle_sequence_line(fasta_reader_base &reader, std::string_view const &sv) = 0;
		virtual bool handle_sequence_end(fasta_reader_base &reader) = 0;
	};
	
	
	// Suitable for reading whole files, i.e. no random access.
	class fasta_reader_base
	{
	public:
		typedef file_handle	handle_type;
		
		enum class parsing_status {
			success,
			failure,
			cancelled
		};
		
	protected:
		struct fsm
		{
			char const	*p{nullptr};
			char const	*pe{nullptr};
			char const	*eof{nullptr};
		};
		
		struct range
		{
			std::size_t pos{};
			std::size_t end{};
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
		std::size_t								m_lineno{};
	
	protected:
		virtual void report_unexpected_character(char const *line_start, std::size_t const lineno, int const current_state) const = 0;
		virtual void report_unexpected_eof(char const *line_start, std::size_t const lineno, int const current_state) const = 0;
		
	public:
		parsing_status parse(handle_type &handle, fasta_reader_delegate &delegate, std::size_t blocksize);
		parsing_status parse(handle_type &handle, fasta_reader_delegate &delegate) { return parse(handle, delegate, handle.io_op_blocksize()); }
	};
	
	
	class fasta_reader final : public fasta_reader_base
	{
	private:
		[[noreturn]] void report_unexpected_character(char const *line_start, std::size_t const lineno, int const current_state) const override;
		[[noreturn]] void report_unexpected_eof(char const *line_start, std::size_t const lineno, int const current_state) const override;
		void output_buffer_end(char const *line_start) const;
	};
	
	
	bool read_single_fasta_sequence(char const *fasta_path, std::vector <char> &seq, char const *seq_name = nullptr);
}

#endif

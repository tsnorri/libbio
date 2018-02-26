/*
 * Copyright (c) 2017-2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_READER_HH
#define LIBBIO_VCF_READER_HH

#include <istream>
#include <map>
#include <vector>
#include <libbio/copyable_atomic.hh>
#include <libbio/variant.hh>
#include <libbio/vcf_input.hh>


namespace libbio {
	
	class vcf_mmap_input;
	
	
	class vcf_reader
	{
		friend class vcf_stream_input_base;
		friend class vcf_mmap_input;
		
	public:
		typedef std::function <bool(transient_variant const &var)> callback_fn;
		typedef std::map <std::string, std::size_t> sample_name_map;
		
	protected:
		struct fsm
		{
			char const	*p{nullptr};
			char const	*pe{nullptr};
			char const	*eof{nullptr};
		};
		
		template <typename> struct caller;
		template <typename> friend struct caller;
		
		template <int t_continue, int t_break>
		int check_max_field(vcf_field const field, int const target, callback_fn const &cb);

		void report_unexpected_character(char const *current_character, int const current_state);

	protected:
		vcf_input						*m_input{nullptr};
		fsm								m_fsm;
		transient_variant				m_current_variant;
		sample_name_map					m_sample_names;
		std::vector <format_field>		m_format;
		char const						*m_line_start{nullptr};		// Current line start.
		char const						*m_start{0};				// Current string start.
		copyable_atomic <std::size_t>	m_counter{0};
		std::size_t						m_lineno{0};				// Current line number.
		std::size_t						m_variant_index{0};			// Current variant number (0-based).
		std::size_t						m_sample_idx{0};			// Current sample idx (1-based).
		std::size_t						m_idx{0};					// Current index in multi-part fields.
		std::size_t						m_format_idx{0};
		std::size_t						m_integer{0};				// Currently read from the input.
		sv_type							m_alt_sv{sv_type::NONE};	// Current ALT structural variant type.
		vcf_field						m_max_parsed_field{};
		bool							m_gt_is_phased{false};		// Is the current GT phased.
		bool							m_alt_is_complex{false};	// Is the current ALT “complex” (includes *).
	
	public:
		vcf_reader() = default;
		
		vcf_reader(vcf_input &input):
			m_input(&input)
		{
		}
		
		void set_input(vcf_input &input) { m_input = &input; }
		void read_header();
		void fill_buffer();
		void reset();
		bool parse(callback_fn &&callback);
		bool parse(callback_fn const &callback);
		
		class vcf_input &vcf_input() { return *m_input; }
		class vcf_input const &vcf_input() const { return *m_input; }
		char const *buffer_start() const { return m_fsm.p; }
		char const *buffer_end() const { return m_fsm.pe; }
		char const *eof() const { return m_fsm.eof; }
		
		std::size_t lineno() const { return m_lineno; }
		std::size_t last_header_lineno() const { return m_input->last_header_lineno(); }
		char const *line_start() const { return m_line_start; }
		std::size_t sample_no(std::string const &sample_name) const;
		std::size_t sample_count() const { return m_sample_names.size(); }
		sample_name_map const &sample_names() const { return m_sample_names; }
		void set_parsed_fields(vcf_field max_field) { m_max_parsed_field = max_field; }
		std::size_t counter_value() const { return m_counter; } // Thread-safe.
		
	protected:
		void skip_to_next_nl();
		void set_buffer_start(char const *p) { m_fsm.p = p; }
		void set_buffer_end(char const *pe) { m_fsm.pe = pe; }
		void set_eof(char const *eof) { m_fsm.eof = eof; }
		void set_lineno(std::size_t const lineno) { assert(lineno); m_lineno = lineno - 1; }
	};
}

#endif

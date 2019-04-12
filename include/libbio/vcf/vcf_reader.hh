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
#include <libbio/vcf/variant_decl.hh>
#include <libbio/vcf/vcf_input.hh>
#include <libbio/vcf/vcf_metadata_decl.hh>


namespace libbio {
	
	class vcf_mmap_input;
	class transient_variant;
	
	
	class vcf_reader
	{
		friend class vcf_stream_input_base;
		friend class vcf_mmap_input;
		friend class variant_base;
		
	public:
		typedef std::map <std::string, std::size_t>			sample_name_map;
		
		struct parsing_delegate
		{
			virtual ~parsing_delegate() {}
			virtual bool handle_variant(vcf_reader &reader, transient_variant const &var) = 0;
			virtual void reader_will_update_format(vcf_reader &reader) {};
			virtual void reader_did_update_format(vcf_reader &reader) {};
		};
		
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
		int check_max_field(vcf_field const field, int const target, parsing_delegate &cb);

		void report_unexpected_character(char const *current_character, std::size_t const pos, int const current_state);

	protected:
		class vcf_input					*m_input{nullptr};
		fsm								m_fsm;
		vcf_metadata					m_metadata;
		vcf_info_field_map				m_info_fields;
		vcf_genotype_field_map			m_genotype_fields;
		vcf_format_ptr_vector			m_current_format;
		sample_name_map					m_sample_names;
		transient_variant				m_current_variant;
		copyable_atomic <std::size_t>	m_counter{0};
		std::size_t						m_lineno{1};				// Current line number.
		std::size_t						m_variant_index{0};			// Current variant number (0-based).
		vcf_field						m_max_parsed_field{};
	
	public:
		vcf_reader() = default;
		
		vcf_reader(class vcf_input &input):
			m_input(&input)
		{
		}
		
		void set_input(class vcf_input &input) { m_input = &input; }
		void read_header();
		void fill_buffer();
		void reset();
		bool parse(parsing_delegate &delegate);
		
		class vcf_input &vcf_input() { return *m_input; }
		class vcf_input const &vcf_input() const { return *m_input; }
		char const *buffer_start() const { return m_fsm.p; }
		char const *buffer_end() const { return m_fsm.pe; }
		char const *eof() const { return m_fsm.eof; }
		
		vcf_metadata const &metadata() const { return m_metadata; }
		vcf_info_field_map &info_fields() { return m_info_fields; }
		vcf_genotype_field_map &genotype_fields() { return m_genotype_fields; }
		vcf_format_ptr_vector const &current_format() { return m_current_format; }
		
		std::size_t lineno() const { return m_lineno; }
		std::size_t last_header_lineno() const { return m_input->last_header_lineno(); }
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
		void set_lineno(std::size_t const lineno) { libbio_assert(lineno); m_lineno = lineno - 1; }

		void associate_metadata_with_field_descriptions();
		std::pair <std::uint16_t, std::uint16_t> assign_field_offsets(std::vector <vcf_subfield_base *> &field_vec) const;
		std::pair <std::uint16_t, std::uint16_t> assign_info_field_offsets() const;
		std::pair <std::uint16_t, std::uint16_t> assign_format_field_offsets() const;
		void parse_format(std::string_view const &new_format);
		
		void initialize_variant(variant_base &variant);
		void deinitialize_variant(variant_base &variant);
		void initialize_variant_samples(variant_base &variant);
		void deinitialize_variant_samples(variant_base &variant);
		void reserve_memory_for_samples_in_current_variant(std::uint16_t const size, std::uint16_t const alignment);
		void copy_variant(variant_base const &src, variant_base &dst);
	};
}

#include <libbio/vcf/variant_def.hh>

#endif

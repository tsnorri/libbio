/*
 * Copyright (c) 2017-2022 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_READER_DECL_HH
#define LIBBIO_VCF_READER_DECL_HH

#include <istream>
#include <map>
#include <vector>
#include <libbio/copyable_atomic.hh>
#include <libbio/utility.hh>
#include <libbio/vcf/metadata.hh>
#include <libbio/vcf/subfield/decl.hh>
#include <libbio/vcf/subfield/genotype_field_base_decl.hh>
#include <libbio/vcf/subfield/info_field_base_decl.hh>
#include <libbio/vcf/variant_format.hh>
#include <libbio/vcf/variant/variant_decl.hh>
#include <libbio/vcf/vcf_input.hh>


namespace libbio::vcf {
	class reader;
	
	
	enum class variant_validation_result
	{
		PASS,
		SKIP,
		STOP
	};
	
	
	struct reader_delegate
	{
		virtual ~reader_delegate() {}
		virtual void vcf_reader_did_parse_metadata(reader &vcf_reader) = 0;
		virtual bool vcf_reader_should_replace_non_matching_subfield(std::string const &, info_field_base const &, metadata_info const &) = 0;
		virtual bool vcf_reader_should_replace_non_matching_subfield(std::string const &, genotype_field_base const &, metadata_format const &) = 0;
	};
	
	struct reader_default_delegate : public reader_delegate
	{
		void vcf_reader_did_parse_metadata(reader &vcf_reader) override {}
		bool vcf_reader_should_replace_non_matching_subfield(std::string const &, info_field_base const &, metadata_info const &) override;
		bool vcf_reader_should_replace_non_matching_subfield(std::string const &, genotype_field_base const &, metadata_format const &) override;
	};
	
	
	struct variant_validator
	{
		virtual ~variant_validator() {}
		virtual variant_validation_result validate(transient_variant const &variant) = 0;
	};
	
	struct variant_no_op_validator : public variant_validator
	{
		variant_validation_result validate(transient_variant const &variant) override { return variant_validation_result::PASS; }
	};
}

namespace libbio::vcf::detail {
	extern reader_default_delegate	g_vcf_reader_default_delegate;
	extern variant_no_op_validator	g_vcf_reader_default_variant_validator;
}


namespace libbio::vcf {
	
	class transient_variant;
	
	
	class reader
	{
		friend class empty_input;
		friend class stream_input_base;
		friend class mmap_input;
		
		friend class variant_format_access;
		friend class transient_variant_format_access;
		
		template <typename, typename>
		friend class formatted_variant;
		
	public:
		typedef std::function <bool(transient_variant &var)>						callback_fn;
		typedef std::function <bool(transient_variant const &var)>					callback_cq_fn;
		typedef std::vector <std::string>											sample_name_vector;
		typedef std::map <std::string, std::size_t, compare_strings_transparent>	sample_name_map; // Use a transparent comparator.
		
		// Enough state information to make it possible to resume parsing from the start of a line.
		// TODO: consider moving more (most?) state information here since reset() will affect the machine state anyway.
		struct parser_state
		{
			friend reader;
			
		protected:
			std::string	current_format;	// Current format as a string.
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
		
		template <int t_continue, int t_break, bool t_should_validate_chrom_and_pos = false, typename t_cb>
		inline int next_state(field const vcf_field, int const target, bool const stop_after_parsing, t_cb const &cb, bool &retval);
		
		void report_unexpected_character(char const *current_character, std::size_t const pos, int const current_state, bool const in_header = false);

	protected:
		input_base						*m_input{nullptr};
		fsm								m_fsm;
		class metadata					m_metadata;
		info_field_map					m_info_fields;
		info_field_ptr_vector			m_info_fields_in_headers;
		info_field_ptr_vector			m_current_record_info_fields;
		genotype_field_map				m_genotype_fields;
		variant_format_ptr				m_current_format{new variant_format()};
		genotype_ptr_vector				m_current_format_vec;					// Non-owning, contents point to m_current_format’s fields.
		sample_name_vector				m_sample_names_by_index;
		sample_name_map					m_sample_indices_by_name;
		transient_variant				m_current_variant;						// FIXME: Consider moving to parser_state. On the other hand, most of the data members have to do with state.
		reader_delegate					*m_delegate{&detail::g_vcf_reader_default_delegate};
		variant_validator				*m_chrom_pos_validator{&detail::g_vcf_reader_default_variant_validator};
		char const						*m_current_line_or_buffer_start{};
		copyable_atomic <std::size_t>	m_counter{0};
		std::size_t						m_lineno{0};							// Current line number.
		std::size_t						m_variant_index{0};						// Current variant number (0-based).
		field							m_max_parsed_field{};
		bool							m_have_assigned_variant_format{};
		bool							m_has_samples{};
	
	public:
		reader() = default;
		
		inline reader(input_base &vcf_input);
		
		void set_delegate(reader_delegate &delegate) { m_delegate = &delegate; }
		void set_variant_validator(variant_validator &validator) { m_chrom_pos_validator = &validator; }
		inline void set_input(input_base &input);
		void set_variant_format(variant_format *fmt) { libbio_always_assert(fmt); m_current_format.reset(fmt); m_have_assigned_variant_format = true; }
		void read_header();
		void parse_nc(callback_fn const &callback);	// Callback takes non-const transient_variant.
		void parse_nc(callback_fn &&callback);		// Callback takes non-const transient_variant.
		void parse(callback_cq_fn const &callback);
		void parse(callback_cq_fn &&callback);
		bool parse_one_nc(callback_fn const &callback, parser_state &state);
		bool parse_one_nc(callback_fn &&callback, parser_state &state);
		bool parse_one(callback_cq_fn const &callback, parser_state &state);
		bool parse_one(callback_cq_fn &&callback, parser_state &state);
		
		bool has_input() const { return (m_input ? true : false); }
		input_base &vcf_input() { return *m_input; }
		input_base const &vcf_input() const { return *m_input; }
		char const *buffer_start() const { return m_fsm.p; }
		char const *buffer_end() const { return m_fsm.pe; }
		char const *eof() const { return m_fsm.eof; }
		
		bool has_assigned_variant_format() const { return m_have_assigned_variant_format; }
		class metadata &metadata() { return m_metadata; }
		class metadata const &metadata() const { return m_metadata; }
		info_field_map &info_fields() { return m_info_fields; }
		info_field_map const &info_fields() const { return m_info_fields; }
		info_field_ptr_vector const &info_fields_in_headers() const { return m_info_fields_in_headers; }
		info_field_ptr_vector const &current_record_info_fields() const { return m_current_record_info_fields; }
		
		genotype_field_map &genotype_fields() { return m_genotype_fields; }
		genotype_field_map const &genotype_fields() const { return m_genotype_fields; }
		variant_format const &get_variant_format() const { return *m_current_format; }
		variant_format_ptr const &get_variant_format_ptr() const { return m_current_format; }
		genotype_ptr_vector const &get_current_variant_format() const { return m_current_format_vec; }
		
		inline variant make_empty_variant();
		transient_variant &current_variant() { return m_current_variant; }
		transient_variant const &current_variant() const { return m_current_variant; }
		
		std::size_t lineno() const { return m_lineno; }
		std::size_t last_header_lineno() const { return m_input->last_header_lineno(); }
		std::size_t sample_no(std::string const &sample_name) const;
		std::size_t sample_count() const { return m_sample_names_by_index.size(); }
		sample_name_vector const &sample_names_by_index() const { return m_sample_names_by_index; }
		sample_name_map const &sample_indices_by_name() const { return m_sample_indices_by_name; }
		inline void set_parsed_fields(field max_field);
		std::size_t counter_value() const { return m_counter; } // Thread-safe.
		inline std::string_view buffer_tail() const; // Valid in parse()’s callback.
		
		template <typename t_key, typename t_dst>
		inline void get_info_field_ptr(t_key const &key, t_dst &dst) const;
		
		template <typename t_key, typename t_dst>
		inline void get_genotype_field_ptr(t_key const &key, t_dst &dst) const;
		
		inline info_field_end *get_end_field_ptr() const;
		
	protected:
		void fill_buffer();
		
		void skip_to_next_nl();
		void set_buffer_start(char const *p) { m_fsm.p = p; }
		void set_buffer_end(char const *pe) { m_fsm.pe = pe; }
		void set_eof(char const *eof) { m_fsm.eof = eof; }
		void set_lineno(std::size_t const lineno) { libbio_assert(lineno); m_lineno = lineno - 1; }
		
		void associate_metadata_with_field_descriptions();
		
		template <typename t_cb>
		inline bool parse2(t_cb const &cb, parser_state &state, bool const stop_after_newline);
		
		std::pair <std::uint16_t, std::uint16_t> assign_info_field_offsets();
		std::pair <std::uint16_t, std::uint16_t> assign_format_field_indices_and_offsets();
		void parse_format(std::string_view const &new_format);
	};
}

#endif

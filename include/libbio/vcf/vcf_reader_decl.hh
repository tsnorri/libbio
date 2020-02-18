/*
 * Copyright (c) 2017-2020 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_READER_DECL_HH
#define LIBBIO_VCF_READER_DECL_HH

#include <istream>
#include <map>
#include <vector>
#include <libbio/copyable_atomic.hh>
#include <libbio/utility.hh>
#include <libbio/vcf/variant_decl.hh>
#include <libbio/vcf/variant_format.hh>
#include <libbio/vcf/vcf_input.hh>
#include <libbio/vcf/vcf_metadata.hh>
#include <libbio/vcf/vcf_subfield_decl.hh>


namespace libbio {
	class vcf_reader;

	struct vcf_reader_delegate
	{
		virtual void vcf_reader_did_parse_metadata(vcf_reader &reader) = 0;
	};

	struct vcf_reader_default_delegate : public vcf_reader_delegate
	{
		void vcf_reader_did_parse_metadata(vcf_reader &reader) {}
	};
}

namespace libbio { namespace detail {
	extern vcf_reader_default_delegate g_vcf_reader_default_delegate;
}}


namespace libbio {
	
	class vcf_mmap_input;
	class transient_variant;
	
	
	class vcf_reader
	{
		friend class vcf_stream_input_base;
		friend class vcf_mmap_input;
		friend class variant_base;
		friend class variant_format_access;
		friend class transient_variant_format_access;
		
		template <typename>
		friend class formatted_variant_base;
		
	public:
		typedef std::function <bool(transient_variant &var)>						callback_fn;
		typedef std::function <bool(transient_variant const &var)>					callback_cq_fn;
		typedef std::map <std::string, std::size_t, compare_strings_transparent>	sample_name_map; // Use a transparent comparator.
		
		// Enough state information to make it possible to resume parsing from the start of a line.
		// TODO: consider moving more (most?) state information here since reset() will affect the machine state anyway.
		struct parser_state
		{
			friend vcf_reader;
			
		protected:
			std::string	current_format;	// Current format as a string.
			std::size_t	lineno{1};
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
		
		template <int t_continue, int t_break, typename t_cb>
		inline int check_max_field(vcf_field const field, int const target, bool const stop_after_parsing, t_cb const &cb, bool &retval);
		
		void report_unexpected_character(char const *current_character, std::size_t const pos, int const current_state);

	protected:
		class vcf_input					*m_input{nullptr};
		fsm								m_fsm;
		vcf_metadata					m_metadata;
		vcf_info_field_map				m_info_fields;
		vcf_info_field_ptr_vector		m_info_fields_in_headers;
		vcf_genotype_field_map			m_genotype_fields;
		variant_format_ptr				m_current_format{new variant_format()};
		vcf_genotype_ptr_vector			m_current_format_vec;					// Non-owning, contents point to m_current_format’s fields.
		sample_name_map					m_sample_names;
		transient_variant				m_current_variant;
		vcf_reader_delegate				*m_delegate{&detail::g_vcf_reader_default_delegate};
		char const						*m_current_line_start{};
		copyable_atomic <std::size_t>	m_counter{0};
		std::size_t						m_lineno{1};							// Current line number.
		std::size_t						m_variant_index{0};						// Current variant number (0-based).
		vcf_field						m_max_parsed_field{};
		bool							m_have_assigned_variant_format{};
		bool							m_has_samples{};
	
	public:
		vcf_reader() = default;
		
		vcf_reader(class vcf_input &input):
			m_input(&input)
		{
		}
		
		void set_delegate(vcf_reader_delegate &delegate) { m_delegate = &delegate; }
		void set_input(class vcf_input &input) { m_input = &input; }
		void set_variant_format(variant_format *fmt) { libbio_always_assert(fmt); m_current_format.reset(fmt); m_have_assigned_variant_format = true; }
		void read_header();
		void fill_buffer();
		void reset();
		bool parse_nc(callback_fn const &callback);
		bool parse_nc(callback_fn &&callback);
		bool parse(callback_cq_fn const &callback);
		bool parse(callback_cq_fn &&callback);
		bool parse_one_nc(callback_fn const &callback, parser_state &state);
		bool parse_one_nc(callback_fn &&callback, parser_state &state);
		bool parse_one(callback_cq_fn const &callback, parser_state &state);
		bool parse_one(callback_cq_fn &&callback, parser_state &state);
		
		class vcf_input &vcf_input() { return *m_input; }
		class vcf_input const &vcf_input() const { return *m_input; }
		char const *buffer_start() const { return m_fsm.p; }
		char const *buffer_end() const { return m_fsm.pe; }
		char const *eof() const { return m_fsm.eof; }
		
		bool has_assigned_variant_format() const { return m_have_assigned_variant_format; }
		vcf_metadata &metadata() { return m_metadata; }
		vcf_metadata const &metadata() const { return m_metadata; }
		vcf_info_field_map &info_fields() { return m_info_fields; }
		vcf_info_field_map const &info_fields() const { return m_info_fields; }
		vcf_info_field_ptr_vector const &info_fields_in_headers() const { return m_info_fields_in_headers; }
		vcf_genotype_field_map &genotype_fields() { return m_genotype_fields; }
		vcf_genotype_field_map const &genotype_fields() const { return m_genotype_fields; }
		variant_format const &get_variant_format() const { return *m_current_format; }
		variant_format_ptr const &get_variant_format_ptr() const { return m_current_format; }
		
		std::size_t lineno() const { return m_lineno; }
		std::size_t last_header_lineno() const { return m_input->last_header_lineno(); }
		std::size_t sample_no(std::string const &sample_name) const;
		std::size_t sample_count() const { return m_sample_names.size(); }
		sample_name_map const &sample_names() const { return m_sample_names; }
		inline void set_parsed_fields(vcf_field max_field);
		std::size_t counter_value() const { return m_counter; } // Thread-safe.
		inline std::pair <std::size_t, std::size_t> current_line_range() const; // Valid in parse()’s callback. FIXME: make this work also for the stream input.
		
		template <typename t_key, typename t_dst>
		inline void get_info_field_ptr(t_key const &key, t_dst &dst) const;
		
		template <typename t_key, typename t_dst>
		inline void get_genotype_field_ptr(t_key const &key, t_dst &dst) const;
		
		inline vcf_info_field_end *get_end_field_ptr() const;
		
	protected:
		void skip_to_next_nl();
		void set_buffer_start(char const *p) { m_fsm.p = p; }
		void set_buffer_end(char const *pe) { m_fsm.pe = pe; }
		void set_eof(char const *eof) { m_fsm.eof = eof; }
		void set_lineno(std::size_t const lineno) { libbio_assert(lineno); m_lineno = lineno - 1; }
		
		void associate_metadata_with_field_descriptions();
		
		template <typename t_field>
		std::pair <std::uint16_t, std::uint16_t> assign_field_offsets(std::vector <t_field *> &field_vec) const;
		
		template <typename t_cb>
		inline bool parse2(t_cb const &cb, parser_state &state, bool const stop_after_newline);
		
		std::pair <std::uint16_t, std::uint16_t> assign_info_field_offsets();
		std::pair <std::uint16_t, std::uint16_t> assign_format_field_offsets();
		void parse_format(std::string_view const &new_format);
		
		template <template <std::int32_t, vcf_metadata_value_type> typename t_field_tpl, vcf_metadata_value_type t_field_type, typename t_field_base_class>
		auto instantiate_field(vcf_metadata_formatted_field const &meta) -> t_field_base_class *;
		
		template <template <std::int32_t, vcf_metadata_value_type> typename t_field_tpl, typename t_key, typename t_field_map>
		auto find_or_add_field(vcf_metadata_formatted_field const &meta, t_key const &key, t_field_map &map, bool &did_add) ->
			typename t_field_map::mapped_type::element_type &;
		
		void initialize_variant(variant_base &variant, vcf_genotype_field_map const &format);
		void deinitialize_variant(variant_base &variant, vcf_genotype_field_map const &format);
		void initialize_variant_samples(variant_base &variant, vcf_genotype_field_map const &format);
		void deinitialize_variant_samples(variant_base &variant, vcf_genotype_field_map const &format);
		void reserve_memory_for_samples_in_current_variant(std::uint16_t const size, std::uint16_t const alignment);
		void copy_variant(variant_base const &src, variant_base &dst, vcf_genotype_field_map const &format);
	};
}

#include <libbio/vcf/variant_def.hh>

#endif

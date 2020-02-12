/*
 * Copyright (c) 2017-2019 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VARIANT_DECL_HH
#define LIBBIO_VARIANT_DECL_HH

#include <libbio/buffer.hh>
#include <libbio/cxxcompat.hh>
#include <libbio/types.hh>
#include <libbio/vcf/variant_format.hh>
#include <libbio/vcf/vcf_metadata.hh>
#include <libbio/vcf/vcf_subfield_decl.hh>
#include <vector>


namespace libbio {
	
	class vcf_reader;
	
	
	// Genotype value for one chromosome copy.
	struct sample_genotype
	{
		enum { NULL_ALLELE = (1 << 15) - 1 };
		
		std::uint16_t is_phased : 1, alt : 15;
		
		sample_genotype():
			is_phased(false),
			alt(0)
		{
		}
		
		sample_genotype(std::uint16_t const alt_, bool const is_phased_):
			is_phased(is_phased_),
			alt(alt_)
		{
		}
	};
	
	
	class variant_sample
	{
		friend class vcf_reader;
		friend class vcf_genotype_field_base;
		friend class vcf_genotype_field_gt;
		
		template <std::int32_t, vcf_metadata_value_type>
		friend class vcf_generic_genotype_field_base;
		
	protected:
		std::vector <sample_genotype>						m_genotype;			// FIXME: replace with Boost.Container::small_vector.
		aligned_buffer <std::byte, buffer_base::zero_tag>	m_sample_data{};	// FIXME: if the range contains only trivially constructible and destructible values, copy the bytes.
	};
	
	
	// Declare the constructors here s.t. the initialization code may be written more easily.
	class variant_base
	{
		friend class vcf_reader;
		friend class vcf_info_field_base;
		friend class variant_format_access;
		friend class transient_variant_format_access;
		
		template <std::int32_t, vcf_metadata_value_type>
		friend class vcf_generic_info_field_base;
		
	public:
		enum { UNKNOWN_QUALITY = -1 };
		typedef std::vector <vcf_metadata_filter const *>	filter_ptr_vector;
		
	protected:
		vcf_reader											*m_reader{};
		aligned_buffer <std::byte, buffer_base::zero_tag>	m_info{};			// FIXME: if the range contains only trivially constructible and destructible values, copy the bytes.
		std::vector <variant_sample>						m_samples{};
		filter_ptr_vector									m_filters{};
		std::vector <bool>									m_assigned_info_fields{};
		double												m_qual{UNKNOWN_QUALITY};
		std::size_t											m_variant_index{0};
		std::size_t											m_lineno{0};
		std::size_t											m_pos{0};
		
	public:
		// Make sure that both m_info and m_samples have zero return value for size(), see the formatters’ destructors.
		variant_base() = default;
		
		inline variant_base(vcf_reader &reader, std::size_t const sample_count, std::size_t const info_size, std::size_t const info_alignment);
		
		void set_variant_index(std::size_t const idx)					{ m_variant_index = idx; }
		void set_lineno(std::size_t const lineno)						{ m_lineno = lineno; }
		void set_pos(std::size_t const pos)								{ m_pos = pos; }
		void set_qual(double const qual)								{ m_qual = qual; }
		
		vcf_reader *reader() const										{ return m_reader; }
		filter_ptr_vector const &filters() const						{ return m_filters; }
		std::vector <variant_sample> const &samples() const				{ return m_samples; }
		std::vector <variant_sample> &samples()							{ return m_samples; }
		std::size_t variant_index() const								{ return m_variant_index; }
		std::size_t lineno() const										{ return m_lineno; }
		std::size_t pos() const											{ return m_pos; }
		inline std::size_t zero_based_pos() const;
		double qual() const												{ return m_qual; }
		
	protected:
		inline void reset();
		void finish_copy(variant_base const &other, variant_format const &variant_format, bool should_initialize);
	};
	
	
	// Use static polymorphism to delegate construction, destruction and copying.
	class transient_variant_format_access
	{
		friend class variant_base;
		friend class variant_format_access;
		
		template <typename>
		friend class formatted_variant_base;
		
		transient_variant_format_access() = default;
		transient_variant_format_access(vcf_reader const &reader) {};
		inline variant_format_ptr const &get_format_ptr(vcf_reader const &reader) const;
	};
	
	class variant_format_access
	{
		friend class variant_base;
		
		template <typename t_base>
		friend class formatted_variant_base;

	protected:
		variant_format_ptr	m_format;
		
	protected:
		variant_format_access() = default;
		inline variant_format_access(vcf_reader const &reader);
		
		variant_format_access(vcf_reader const &reader, transient_variant_format_access const &other):
			m_format(other.get_format_ptr(reader))
		{
		}
		
		variant_format_ptr const &get_format_ptr(vcf_reader const &reader) const { return m_format; }
	};
	
	
	// Override constructors, destructor and copy assignment to delegate the operatrions.
	// Destructors of base classes are called in reverse order of declaration, so t_format_access’s destructor is called before variant_base’s.
	template <typename t_format_access>
	class formatted_variant_base : public variant_base, public t_format_access
	{
	public:
		formatted_variant_base() = default;
		
		inline formatted_variant_base(vcf_reader &reader, std::size_t const sample_count, std::size_t const info_size, std::size_t const info_alignment);
		
		// Copy constructors.
		inline formatted_variant_base(formatted_variant_base const &other);
		
		template <typename t_other_format_access>
		inline formatted_variant_base(formatted_variant_base <t_other_format_access> const &other);
		
		// Move constructor.
		formatted_variant_base(formatted_variant_base &&) = default;
		
		// Copy assignment operator.
		inline formatted_variant_base &operator=(formatted_variant_base const &) &;
		
		// Move assignment operator.
		formatted_variant_base &operator=(formatted_variant_base &&) & = default;
		
		// Destructor.
		inline virtual ~formatted_variant_base();
		
		variant_format_ptr const &get_format_ptr() const { return t_format_access::get_format_ptr(*this->m_reader); }
		variant_format const &get_format() const { return *get_format_ptr(); }
	};
	
	
	struct variant_alt_base
	{
		sv_type		alt_sv_type{};
	};
	
	
	template <typename t_string>
	struct variant_alt : public variant_alt_base
	{
		t_string	alt{};
		
		variant_alt() = default;
		
		// Allow copying from another specialization of variant_alt.
		template <typename t_other_string>
		variant_alt(variant_alt <t_other_string> const &other):
			variant_alt_base(other),
			alt(other.alt)
		{
		}
		
		void set_alt(std::string_view const &alt_) { alt = alt_; }
	};
	
	
	// Store the string-type variant fields as t_string.
	// This class is separated from formatted_variant_base mainly because maintaining
	// constructors etc. with all these variables would be cumbersome.
	template <typename t_string, typename t_format_access>
	class variant_tpl : public formatted_variant_base <t_format_access>
	{
		template <typename, typename>
		friend class variant_tpl;
		
		friend class vcf_reader;
		
	public:
		typedef t_string						string_type;
		typedef t_format_access					format_access;
		
	protected:
		string_type								m_chrom_id{};
		string_type								m_ref{};
		std::vector <string_type>				m_id{};
		std::vector <variant_alt <string_type>>	m_alts{};
		
	public:
		using formatted_variant_base <t_format_access>::formatted_variant_base;
		
		// Allow copying from from another specialization of variant_tpl.
		template <typename t_other_string, typename t_other_format_access>
		variant_tpl(variant_tpl <t_other_string, t_other_format_access> const &other):
			formatted_variant_base <t_format_access>(other),
			m_chrom_id(other.m_chrom_id),
			m_ref(other.m_ref),
			m_id(other.m_id.cbegin(), other.m_id.cend()),
			m_alts(other.m_alts.cbegin(), other.m_alts.cend())
		{
		}
		
		string_type const &chrom_id() const { return m_chrom_id; }
		string_type const &ref() const { return m_ref; }
		std::vector <string_type> const &id() const { return m_id; }
		std::vector <variant_alt <string_type>> const &alts() const { return m_alts; }
		
		void set_chrom_id(std::string_view const &chrom_id) { m_chrom_id = chrom_id; }
		void set_ref(std::string_view const &ref) { m_ref = ref; }
		void set_id(std::string_view const &id, std::size_t const pos);
		void set_alt(std::string_view const &alt, std::size_t const pos);
	};
	
	
	class transient_variant final : public variant_tpl <std::string_view, transient_variant_format_access>
	{
		friend class variant;
		
	public:
		typedef std::string_view				string_type;
		typedef transient_variant_format_access	format_access;
		
	public:
		using variant_tpl <std::string_view, transient_variant_format_access>::variant_tpl;
		
		inline void reset();
	};
	
	
	class variant final : public variant_tpl <std::string, variant_format_access>
	{
	public:
		typedef std::string				string_type;
		typedef variant_format_access	format_access;
		
	public:
		using variant_tpl <std::string, variant_format_access>::variant_tpl;
	};
}

#endif

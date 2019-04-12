/*
 * Copyright (c) 2017-2019 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VARIANT_DECL_HH
#define LIBBIO_VARIANT_DECL_HH

#include <libbio/buffer.hh>
#include <libbio/cxxcompat.hh>
#include <libbio/types.hh>
#include <libbio/vcf/vcf_metadata_decl.hh>
#include <ostream>
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
		friend class vcf_metadata_format;
		friend class vcf_genotype_field_gt;
		
		template <std::int32_t, vcf_metadata_value_type>
		friend class vcf_generic_genotype_field_base;
		
	protected:
		std::vector <sample_genotype>						m_genotype;			// FIXME: replace with Boost.Container::small_vector.
		aligned_buffer <std::byte, buffer_base::zero_tag>	m_sample_data{};	// FIXME: if the range contains only trivially constructible and destructible values, copy the bytes.
	};
	
	
	// Declare the constructors here s.t. the initialization code may be written more easily.
	class variant_data
	{
		friend class vcf_reader;
		
		template <std::int32_t, vcf_metadata_value_type>
		friend class vcf_generic_info_field_base;
		
	protected:
		vcf_reader											*m_reader{};
		aligned_buffer <std::byte, buffer_base::zero_tag>	m_info{};			// FIXME: if the range contains only trivially constructible and destructible values, copy the bytes.
		std::vector <variant_sample>						m_samples{};
		std::vector <bool>									m_assigned_info_fields{};
		std::size_t											m_variant_index{0};
		std::size_t											m_lineno{0};
		std::size_t											m_pos{0};
		std::size_t											m_qual{SIZE_MAX};
		
	public:
		// Make sure that both m_info and m_samples have zero return value for size(), see ~variant_base().
		variant_data() = default;
		
		inline variant_data(vcf_reader &reader, std::size_t const sample_count, std::size_t const info_size, std::size_t const info_alignment);
		
		void set_variant_index(std::size_t const idx)					{ m_variant_index = idx; }
		void set_lineno(std::size_t const lineno)						{ m_lineno = lineno; }
		void set_pos(std::size_t const pos)								{ m_pos = pos; }
		void set_qual(std::size_t const qual)							{ m_qual = qual; }
		
		vcf_reader *reader() const										{ return m_reader; }
		std::vector <variant_sample> const &samples() const				{ return m_samples; }
		std::size_t variant_index() const								{ return m_variant_index; }
		std::size_t lineno() const										{ return m_lineno; }
		std::size_t pos() const											{ return m_pos; }
		inline std::size_t zero_based_pos() const;
		std::size_t qual() const										{ return m_qual; }
		
	protected:
		void reset() { m_assigned_info_fields.assign(m_assigned_info_fields.size(), false); }
	};
	
	
	class variant_base : public variant_data
	{
		friend class vcf_reader;
		friend class vcf_metadata_info;
		
	public:
		enum { UNKNOWN_QUALITY = SIZE_MAX };
		
	public:
		variant_base() = default;
		
		inline variant_base(
			vcf_reader &reader,
			std::size_t const sample_count,
			std::size_t const info_size,
			std::size_t const info_alignment
		);
		
		// Copy constructor.
		inline variant_base(variant_base const &other);
		
		// Move constructor.
		variant_base(variant_base &&) = default;
		
		// Copy assignment operator.
		inline variant_base &operator=(variant_base &other) &;
		
		// Move assignment operator.
		variant_base &operator=(variant_base &&) & = default;
		
		// Destructor.
		inline virtual ~variant_base();
	};
	
	
	template <typename t_string>
	struct variant_alt
	{
		t_string	alt{};
		sv_type		alt_sv_type{};
		
		variant_alt() = default;
		
		// Allow copying from another specialization of variant_alt.
		template <typename t_other_string>
		variant_alt(variant_alt <t_other_string> const &other):
			alt(other.alt),
			alt_sv_type(other.alt_sv_type)
		{
		}
		
		void set_alt(std::string_view const &alt_) { alt = alt_; }
	};
	
	
	template <typename t_string>
	class variant_tpl : public variant_base
	{
		template <typename>
		friend class variant_tpl;
		
		friend class vcf_reader;
		
	protected:
		t_string								m_chrom_id{};
		t_string								m_ref{};
		std::vector <t_string>					m_id{};
		std::vector <variant_alt <t_string>>	m_alts{};
		
	public:
		using variant_base::variant_base;
		
		// Allow copying from from another specialization of variant_tpl.
		template <typename t_other_string>
		variant_tpl(variant_tpl <t_other_string> const &other):
			variant_base(other),
			m_chrom_id(other.m_chrom_id),
			m_ref(other.m_ref),
			m_id(other.m_id.cbegin(), other.m_id.cend()),
			m_alts(other.m_alts.cbegin(), other.m_alts.cend())
		{
		}
		
		t_string const &chrom_id() const { return m_chrom_id; }
		t_string const &ref() const { return m_ref; }
		std::vector <t_string> const &id() const { return m_id; }
		std::vector <variant_alt <t_string>> const &alts() const { return m_alts; }
		
		void set_chrom_id(std::string_view const &chrom_id) { m_chrom_id = chrom_id; }
		void set_ref(std::string_view const &ref) { m_ref = ref; }
		void set_id(std::string_view const &id, std::size_t const pos);
		void set_alt(std::string_view const &alt, std::size_t const pos);
	};
	
	
	class transient_variant final : public variant_tpl <std::string_view>
	{
		friend class variant;
		
	public:
		using variant_tpl <std::string_view>::variant_tpl;
		
		inline void reset();
	};
	
	
	// FIXME: store a copy of the fields here (with pointers to meta?).
	class variant final : public variant_tpl <std::string>
	{
	public:
		using variant_tpl <std::string>::variant_tpl;
	};
	
	
	template <typename t_string>
	void output_vcf(std::ostream &stream, variant_tpl <t_string> const &var, vcf_format_ptr_vector const &format);
}

#endif

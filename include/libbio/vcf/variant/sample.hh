/*
 * Copyright (c) 2020 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_VARIANT_SAMPLE_HH
#define LIBBIO_VCF_VARIANT_SAMPLE_HH

#include <libbio/buffer.hh>
#include <libbio/cxxcompat.hh>
#include <libbio/types.hh>
#include <string>
#include <vector>


namespace libbio::vcf {
	
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
	
	
	class variant_sample_base
	{
		template <typename, typename>
		friend class formatted_variant;
		
		friend class genotype_field_base;
		friend class storable_genotype_field_base;
		
		template <metadata_value_type, std::int32_t>
		friend class generic_genotype_field_base;
		
	protected:
		// The buffer needs to be zeroed when copying b.c. it may contain vectors.
		aligned_buffer <std::byte, buffer_base::zero_tag>	m_sample_data{};	// FIXME: if the range contains only trivially constructible and destructible values, copy the bytes.
		std::vector <bool>									m_assigned_genotype_fields;
		
	protected:
		inline void reset();
		
	public:
		std::vector <bool> const &assigned_genotype_fields() const { return m_assigned_genotype_fields; }
	};
	
	
	// The template parameter is needed for subfields.
	template <typename t_string>
	class variant_sample_tpl : public variant_sample_base
	{
		friend class reader;
		friend class genotype_field_base;
		friend class genotype_field_gt;
		
		template <metadata_value_type, std::int32_t>
		friend class generic_genotype_field_base;
		
	protected:
		typedef t_string string_type;
		
	public:
		variant_sample_tpl() = default;
		variant_sample_tpl(variant_sample_tpl const &) = default;
		variant_sample_tpl(variant_sample_tpl &&) = default;
		variant_sample_tpl &operator=(variant_sample_tpl const &) & = default;
		variant_sample_tpl &operator=(variant_sample_tpl &&) & = default;
		
		// Containing class will handle filling.
		template <typename t_other_string>
		variant_sample_tpl(variant_sample_tpl <t_other_string> const &other):
			variant_sample_base(other)
		{
		}
		
		template <typename t_other_string>
		variant_sample_tpl &operator=(variant_sample_tpl <t_other_string> const &) &;
		
		template <typename t_other_string>
		variant_sample_tpl &operator=(variant_sample_tpl <t_other_string> &&) &;
	};
	
	
	void variant_sample_base::reset()
	{
		m_assigned_genotype_fields.assign(m_assigned_genotype_fields.size(), false);
	}
	
	
	template <typename t_string>
	template <typename t_other_string>
	auto variant_sample_tpl <t_string>::operator=(variant_sample_tpl <t_other_string> const &other) & -> variant_sample_tpl &
	{
		if (static_cast <variant_sample_base const *>(this) != static_cast <variant_sample_base const *>(&other))
			variant_sample_base::operator=(other);
		return *this;
	}
	
	
	template <typename t_string>
	template <typename t_other_string>
	auto variant_sample_tpl <t_string>::operator=(variant_sample_tpl <t_other_string> &&other) & -> variant_sample_tpl &
	{
		if (static_cast <variant_sample_base const *>(this) != static_cast <variant_sample_base const *>(&other))
			variant_sample_base::operator=(other);
		return *this;
	}
	
	
	inline std::ostream &operator<<(std::ostream &os, sample_genotype const &gt)
	{
		// Simple implementation for vcf::vector_value_access_base, use output_vcf_value or output_genotype instead.
		os << gt.alt;
		return os;
	}
	
	
	void output_genotype(std::ostream &stream, std::vector <sample_genotype> const &genotype);
}

#endif

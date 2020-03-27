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


namespace libbio {
	
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
		
		friend class vcf_genotype_field_base;
		
		template <std::int32_t, vcf_metadata_value_type>
		friend class vcf_generic_genotype_field_base;
		
	protected:
		aligned_buffer <std::byte, buffer_base::zero_tag>	m_sample_data{};	// FIXME: if the range contains only trivially constructible and destructible values, copy the bytes.
		std::vector <sample_genotype>						m_genotype;			// FIXME: replace with Boost.Container::small_vector, use m_sample_data to store.
		std::vector <bool>									m_assigned_genotype_fields;
		
	protected:
		inline void reset();
	};
	
	
	// The template parameter is needed for subfields.
	template <typename t_string>
	class variant_sample_tpl : public variant_sample_base
	{
		friend class vcf_reader;
		friend class vcf_genotype_field_base;
		friend class vcf_genotype_field_gt;
		
		template <std::int32_t, vcf_metadata_value_type>
		friend class vcf_generic_genotype_field_base;
		
	protected:
		typedef t_string string_type;
	};
	
	
	void variant_sample_base::reset()
	{
		m_assigned_genotype_fields.assign(m_assigned_genotype_fields.size(), false);
	}
}

#endif

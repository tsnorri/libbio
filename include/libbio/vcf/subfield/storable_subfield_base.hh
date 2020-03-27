/*
 * Copyright (c) 2019-2020 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_SUBFIELD_STORABLE_SUBFIELD_BASE_HH
#define LIBBIO_VCF_SUBFIELD_STORABLE_SUBFIELD_BASE_HH

#include <libbio/vcf/subfield/genotype_field_base_decl.hh>
#include <libbio/vcf/subfield/info_field_base_decl.hh>


namespace libbio {
	
	// Base class for all field types that have an offset. (I.e. not GT.)
	// FIXME: change GT to actually have an offset and move this somewhere else.
	class vcf_storable_subfield_base : public virtual vcf_subfield_base
	{
		friend class vcf_reader;
		
	public:
		enum { INVALID_OFFSET = UINT16_MAX };
		
	protected:
		std::uint16_t		m_offset{INVALID_OFFSET};	// Offset of this field in the memory block.
		
	protected:
		virtual inline std::uint16_t get_offset() const override final { return m_offset; }
		virtual inline void set_offset(std::uint16_t offset) override final { m_offset = offset; }
	};
	
	
	// Base classes for the different field types.
	class vcf_storable_info_field_base : public vcf_storable_subfield_base, public virtual vcf_info_field_base
	{
		friend class vcf_metadata_info;
	};
	
	
	class vcf_storable_genotype_field_base : public vcf_storable_subfield_base, public virtual vcf_genotype_field_base
	{
		friend class vcf_metadata_format;
	};
}

#endif

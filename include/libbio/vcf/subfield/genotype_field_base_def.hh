/*
 * Copyright (c) 2019-2020 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_SUBFIELD_GENOTYPE_FIELD_BASE_DEF_HH
#define LIBBIO_VCF_SUBFIELD_GENOTYPE_FIELD_BASE_DEF_HH

#include <libbio/assert.hh>
#include <libbio/vcf/subfield/genotype_field_base_decl.hh>
#include <libbio/vcf/variant/sample.hh>


namespace libbio::vcf {
	
	void genotype_field_base::prepare(transient_variant_sample &dst) const
	{
		this->reset(dst, dst.m_sample_data.get());
	}
	
	
	void genotype_field_base::parse_and_assign(std::string_view const &sv, transient_variant const &var, transient_variant_sample &dst) const
	{
		// May be called multiple times for a vector subfield.
		libbio_assert(m_metadata);
		libbio_assert_neq(this->get_index(), INVALID_INDEX);
		if (this->parse_and_assign(sv, var, dst, dst.m_sample_data.get()))
			dst.m_assigned_genotype_fields[this->get_index()] = true;
	}
	
	
	bool genotype_field_base::has_value(variant_sample_base const &sample) const
	{
		libbio_assert_neq(this->get_index(), INVALID_INDEX);
		return sample.m_assigned_genotype_fields[this->get_index()];
	}
}

#endif

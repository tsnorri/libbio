/*
 * Copyright (c) 2019 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_METADATA_DEF_HH
#define LIBBIO_VCF_METADATA_DEF_HH

#include <libbio/vcf/variant_decl.hh>
#include <libbio/vcf/vcf_metadata_decl.hh>


namespace libbio {
	
	void vcf_metadata_formatted_field::check_field_type(vcf_metadata_value_type const vt) const
	{
		switch (vt)
		{
			case vcf_metadata_value_type::UNKNOWN:
				throw std::runtime_error("Field contents were to be parsed but field value type was set to unknown");
			
			case vcf_metadata_value_type::FLAG:
				throw std::runtime_error("Field contents were to be parsed but field value type was set to flag");
			
			default:
				break;
		}
	}
	
	
	bool vcf_metadata_info::check_subfield_index(std::int32_t subfield_index) const
	{
		switch (m_number)
		{
			case VCF_NUMBER_UNKNOWN:
			case VCF_NUMBER_ONE_PER_ALTERNATE_ALLELE:
			case VCF_NUMBER_ONE_PER_ALLELE:
			case VCF_NUMBER_ONE_PER_GENOTYPE:
				return true;
			
			default:
				return (subfield_index < m_number);
		}
	}
	
	
	void vcf_metadata_info::prepare(variant_base &dst) const
	{
		m_field->reset(dst.m_info.get());
	}
	
	
	void vcf_metadata_info::parse_and_assign(std::string_view const &sv, transient_variant &dst) const
	{
		check_field_type(m_field->value_type());
		auto const did_assign(m_field->parse_and_assign(sv, dst, dst.m_info.get()));
		dst.m_assigned_info_fields[m_index] = did_assign;
	}
	
	
	void vcf_metadata_info::assign_flag(transient_variant &dst) const
	{
		auto const vt(m_field->value_type());
		if (vcf_metadata_value_type::NOT_PROCESSED == vt)
			return;
		
		if (vcf_metadata_value_type::FLAG != vt)
			throw std::runtime_error("Field type is not FLAG");
		auto const did_assign(m_field->assign(dst.m_info.get()));
		dst.m_assigned_info_fields[m_index] = did_assign;
	}
	
	
	// FIXME: mark the current sample s.t. no format fields have been seen yet (since the user only has all possible format fields by name).
	void vcf_metadata_format::prepare(variant_sample &dst) const
	{
		m_field->reset(dst.m_sample_data.get());
	}
	
	
	void vcf_metadata_format::parse_and_assign(std::string_view const &sv, variant_sample &dst) const
	{
		check_field_type(m_field->value_type());
		m_field->parse_and_assign(sv, dst, dst.m_sample_data.get());
	}
}

#endif

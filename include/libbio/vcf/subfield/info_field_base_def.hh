/*
 * Copyright (c) 2019-2020 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_SUBFIELD_INFO_FIELD_BASE_DEF_HH
#define LIBBIO_VCF_SUBFIELD_INFO_FIELD_BASE_DEF_HH

#include <libbio/vcf/subfield/info_field_base_decl.hh>
#include <libbio/vcf/variant/variant_decl.hh>


namespace libbio::vcf {
	
	void info_field_base::prepare(transient_variant &dst) const
	{
		reset(dst, dst.m_info.get());
	}
	
	
	void info_field_base::parse_and_assign(std::string_view const &sv, transient_variant &dst) const
	{
		// May be called multiple times for a vector subfield.
		libbio_assert(m_metadata);
		auto const did_assign(parse_and_assign(sv, dst, dst.m_info.get()));
		dst.m_assigned_info_fields[m_metadata->get_index()] = did_assign;
	}
	
	
	bool info_field_base::has_value(abstract_variant const &var) const
	{
		libbio_assert(m_metadata);
		return var.m_assigned_info_fields[m_metadata->get_index()];
	}
	
	
	void info_field_base::assign_flag(transient_variant &dst) const
	{
		auto const vt(metadata_value_type());
		if (metadata_value_type::NOT_PROCESSED == vt)
			return;
		
		if (metadata_value_type::FLAG != vt)
			throw std::runtime_error("Field type is not FLAG");
		auto const did_assign(assign(dst.m_info.get()));
		
		libbio_assert(m_metadata);
		libbio_assert_eq(false, dst.m_assigned_info_fields[m_metadata->get_index()]);
		dst.m_assigned_info_fields[m_metadata->get_index()] = did_assign;
	}
	
	
	template <typename t_string, typename t_format_access>
	bool info_field_base::output_vcf_value(
		std::ostream &stream,
		formatted_variant <t_string, t_format_access> const &var,
		char const *sep
	) const
	{
		libbio_assert(m_metadata);
		if (var.m_assigned_info_fields[m_metadata->get_index()])
		{
			stream << sep << m_metadata->get_id();
			if (metadata_value_type::FLAG != metadata_value_type())
			{
				stream << '=';
				output_vcf_value(stream, var);
			}
			return true;
		}
		return false;
	}
}

#endif

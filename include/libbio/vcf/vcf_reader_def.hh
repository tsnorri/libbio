/*
 * Copyright (c) 2019 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_READER_DEF_HH
#define LIBBIO_VCF_READER_DEF_HH

#include <libbio/vcf/vcf_reader_decl.hh>
#include <libbio/vcf/vcf_subfield_def.hh>


namespace libbio {
	
	template <typename t_key, typename t_dst>
	void vcf_reader::get_info_field_ptr(t_key const &key, t_dst &dst) const
	{
		auto const it(m_info_fields.find(key));
		libbio_always_assert_neq(it, m_info_fields.end());
		dst = dynamic_cast <std::remove_reference_t <decltype(dst)>>(it->second.get());
	}
	
	template <typename t_key, typename t_dst>
	void vcf_reader::get_genotype_field_ptr(t_key const &key, t_dst &dst) const
	{
		auto const it(m_genotype_fields.find(key));
		libbio_always_assert_neq(it, m_genotype_fields.end());
		dst = dynamic_cast <std::remove_reference_t <decltype(dst)>>(it->second.get());
	}
	
	vcf_info_field_end *vcf_reader::get_end_field_ptr() const
	{
		vcf_info_field_end *retval{};
		get_info_field_ptr("END", retval);
		return retval;
	}
}

#endif

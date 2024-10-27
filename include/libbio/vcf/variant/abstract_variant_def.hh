/*
 * Copyright (c) 2020-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_VARIANT_ABSTRACT_VARIANT_DEF_HH
#define LIBBIO_VCF_VARIANT_ABSTRACT_VARIANT_DEF_HH

#include <cstddef>
#include <libbio/assert.hh>
#include <libbio/vcf/variant/abstract_variant_decl.hh>
#include <libbio/vcf/vcf_reader_def.hh>


namespace libbio::vcf {

	abstract_variant::abstract_variant(
		class reader &vcf_reader,
		std::size_t const info_size,
		std::size_t const info_alignment
	):
		m_reader(&vcf_reader),
		m_info(info_size, info_alignment),
		m_assigned_info_fields(vcf_reader.metadata().info().size(), false)
	{
	}


	std::size_t abstract_variant::zero_based_pos() const
	{
		libbio_always_assert_msg(0 != m_pos, "Unexpected position");
		return m_pos - 1;
	}


	void abstract_variant::reset()
	{
		m_assigned_info_fields.assign(m_assigned_info_fields.size(), false);
		m_filters.clear();
	}
}

#endif

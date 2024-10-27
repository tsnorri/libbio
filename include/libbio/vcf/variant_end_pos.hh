/*
 * Copyright (c) 2019-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_VARIANT_END_POS_HH
#define LIBBIO_VCF_VARIANT_END_POS_HH

#include <cstddef>
#include <libbio/assert.hh>
#include <libbio/vcf/subfield/decl.hh>
#include <libbio/vcf/subfield/generic_field.hh>
#include <libbio/vcf/variant/variant_tpl.hh>


namespace libbio::vcf {

	template <typename t_string>
	std::size_t variant_end_pos(variant_tpl <t_string> const &var, info_field_end const &end_field)
	{
		if (end_field.get_metadata() && end_field.has_value(var))
			return end_field(var);
		else
			return var.zero_based_pos() + var.ref().size();
	}
}

#endif

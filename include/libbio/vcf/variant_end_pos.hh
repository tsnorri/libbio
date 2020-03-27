/*
 * Copyright (c) 2019 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_VARIANT_END_POS_HH
#define LIBBIO_VCF_VARIANT_END_POS_HH

#include <libbio/assert.hh>
#include <libbio/vcf/subfield/generic_field.hh>
#include <libbio/vcf/variant/variant_tpl.hh>


namespace libbio {
	
	template <typename t_string>
	std::size_t variant_end_pos(variant_tpl <t_string> const &var, vcf_info_field_end const &end_field)
	{
		if (end_field.get_metadata() && end_field.has_value(var))
			return end_field(var);
		else
			return var.zero_based_pos() + var.ref().size();
	}	
}

#endif

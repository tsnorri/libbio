/*
 * Copyright (c) 2020 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_VARIANT_VARIANT_HH
#define LIBBIO_VCF_VARIANT_VARIANT_HH

#include <libbio/vcf/variant/abstract_variant_def.hh>
#include <libbio/vcf/variant/variant_decl.hh>


namespace libbio::vcf {
	
	void transient_variant::reset()
	{
		variant_tpl <std::string_view>::reset();
		
		// Try to avoid deallocating the samples.
		m_id.clear();
		m_alts.clear();
	}
	
	
	variant &variant::operator=(transient_variant const &other) &
	{
		variant_formatted_base::operator=(other);
		return *this;
	}
}

#endif

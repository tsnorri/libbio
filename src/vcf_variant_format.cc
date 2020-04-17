/*
 * Copyright (c) 2020 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <libbio/vcf/variant_format.hh>
#include <range/v3/view/zip.hpp>


namespace lb	= libbio;


namespace libbio::vcf {
	
	bool operator==(variant_format const &lhs, variant_format const &rhs)
	{
		auto const &lhs_fields(lhs.m_fields_by_identifier);
		auto const &rhs_fields(rhs.m_fields_by_identifier);
		if (lhs_fields.size() != rhs_fields.size())
			return false;
		
		// Check that the metadata and offsets match.
		for (auto const &[lhs_kv, rhs_kv] : ranges::view::zip(lhs_fields, rhs_fields))
		{
			auto const &lhs_field(*lhs_kv.second);
			auto const &rhs_field(*rhs_kv.second);
			
			if (!(lhs_field.get_metadata() == rhs_field.get_metadata() && lhs_field.get_offset() == rhs_field.get_offset()))
				return false;
		}
		
		return true;
	}
}

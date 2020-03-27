/*
 * Copyright (c) 2019 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <boost/spirit/include/qi.hpp>
#include <charconv>
#include <libbio/vcf/subfield.hh>


namespace lb	= libbio;


namespace {

	template <typename t_map, typename t_key, typename t_ptr_value>
	void add_to_map(t_map &map, t_key const &key, t_ptr_value const ptr_value)
	{
		map.emplace(
			std::piecewise_construct,
			std::forward_as_tuple(key),
			std::forward_as_tuple(ptr_value)
		);
	}
}


namespace libbio {
	
	bool vcf_subfield_parser <vcf_metadata_value_type::INTEGER>::parse(std::string_view const &sv, value_type &dst)
	{
		auto const *start(sv.data());
		auto const *end(start + sv.size());
		auto const res(std::from_chars(start, end, dst));
		if (std::errc::invalid_argument == res.ec || std::errc::result_out_of_range == res.ec)
		{
			// Check for MISSING.
			if ("." == sv)
				return false;
			
			throw std::runtime_error("Unable to parse the given value");
		}
		return true;
	}
	
	
	bool vcf_subfield_parser <vcf_metadata_value_type::FLOAT>::parse(std::string_view const &sv, value_type &dst)
	{
		// libc++ does not yet have from_chars for float.
#if 0
		auto const *start(sv.data());
		auto const *end(start + sv.size());
		auto const res(std::from_chars(start, end, dst));
		if (std::errc::invalid_argument == res.ec || std::errc::result_out_of_range == res.ec)
		{
			// Check for MISSING.
			if ("." == sv)
				return false;
			
			throw std::runtime_error("Unable to parse the given value");
		}
#endif
		
		if (!boost::spirit::qi::parse(sv.begin(), sv.end(), boost::spirit::qi::float_, dst))
		{
			// Check for MISSING.
			if ("." == sv)
				return false;
			
			throw std::runtime_error("Unable to parse the given value");
		}
		return true;
	}
	
	
	void add_reserved_info_keys(vcf_info_field_map &dst)
	{
		add_to_map(dst, "AA",			new vcf_info_field_aa());
		add_to_map(dst, "AC",			new vcf_info_field_ac());
		add_to_map(dst, "AD",			new vcf_info_field_ad());
		add_to_map(dst, "ADF",			new vcf_info_field_adf());
		add_to_map(dst, "ADR",			new vcf_info_field_adr());
		add_to_map(dst, "AF",			new vcf_info_field_af());
		add_to_map(dst, "AN",			new vcf_info_field_an());
		add_to_map(dst, "BQ",			new vcf_info_field_bq());
		add_to_map(dst, "CIGAR",		new vcf_info_field_cigar());
		add_to_map(dst, "DB",			new vcf_info_field_db());
		add_to_map(dst, "DP",			new vcf_info_field_dp());
		add_to_map(dst, "END",			new vcf_info_field_end());
		add_to_map(dst, "H2",			new vcf_info_field_h2());
		add_to_map(dst, "H3",			new vcf_info_field_h3());
		add_to_map(dst, "MQ",			new vcf_info_field_mq());
		add_to_map(dst, "MQ0",			new vcf_info_field_mq0());
		add_to_map(dst, "NS",			new vcf_info_field_ns());
		add_to_map(dst, "SB",			new vcf_info_field_sb());
		add_to_map(dst, "SOMATIC",		new vcf_info_field_somatic());
		add_to_map(dst, "VALIDATED",	new vcf_info_field_validated());
		add_to_map(dst, "1000G",		new vcf_info_field_1000g());
	}
	
	
	void add_reserved_genotype_keys(vcf_genotype_field_map &dst)
	{
		add_to_map(dst, "AD",			new vcf_genotype_field_ad());
		add_to_map(dst, "ADF",			new vcf_genotype_field_adf());
		add_to_map(dst, "ADR",			new vcf_genotype_field_adr());
		add_to_map(dst, "DP",			new vcf_genotype_field_dp());
		add_to_map(dst, "EC",			new vcf_genotype_field_ec());
		add_to_map(dst, "FT",			new vcf_genotype_field_ft());
		add_to_map(dst, "GL",			new vcf_genotype_field_gl());
		add_to_map(dst, "GP",			new vcf_genotype_field_gp());
		add_to_map(dst, "GQ",			new vcf_genotype_field_gq());
		add_to_map(dst, "GT",			new vcf_genotype_field_gt());
		add_to_map(dst, "HQ",			new vcf_genotype_field_hq());
		add_to_map(dst, "MQ",			new vcf_genotype_field_mq());
		add_to_map(dst, "PL",			new vcf_genotype_field_pl());
		add_to_map(dst, "PQ",			new vcf_genotype_field_pq());
		add_to_map(dst, "PS",			new vcf_genotype_field_ps());
	}
}

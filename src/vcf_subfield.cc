/*
 * Copyright (c) 2019-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <boost/spirit/include/qi.hpp>
#include <charconv>
#include <libbio/vcf/constants.hh>
#include <libbio/vcf/metadata.hh>
#include <libbio/vcf/parse_error.hh>
#include <libbio/vcf/subfield.hh>
#include <libbio/vcf/subfield/utility/parser.hh>
#include <string_view>
#include <system_error>


namespace libbio::vcf {

	bool subfield_parser <metadata_value_type::INTEGER>::parse(std::string_view const &sv, value_type &dst, metadata_formatted_field const *field_ptr)
	{
		auto const *start(sv.data());
		auto const *end(start + sv.size());
		auto const res(std::from_chars(start, end, dst));
		if (std::errc::invalid_argument == res.ec || std::errc::result_out_of_range == res.ec)
		{
			// Check for MISSING.
			if ("." == sv)
				return false;

			throw parse_error("Unable to parse the given value as an integer", sv, field_ptr);
		}
		return true;
	}


	bool subfield_parser <metadata_value_type::FLOAT>::parse(std::string_view const &sv, value_type &dst, metadata_formatted_field const *field_ptr)
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

			throw parse_error("Unable to parse the given value as floating point", sv);
		}
#endif

		if (!boost::spirit::qi::parse(sv.begin(), sv.end(), boost::spirit::qi::float_, dst))
		{
			// Check for MISSING.
			if ("." == sv)
				return false;

			throw parse_error("Unable to parse the given value as floating point", sv, field_ptr);
		}
		return true;
	}


	void add_reserved_info_keys(info_field_map &dst)
	{
		add_subfield <info_field_aa>			(dst, "AA");
		add_subfield <info_field_ac>			(dst, "AC");
		add_subfield <info_field_ad>			(dst, "AD");
		add_subfield <info_field_adf>			(dst, "ADF");
		add_subfield <info_field_adr>			(dst, "ADR");
		add_subfield <info_field_af>			(dst, "AF");
		add_subfield <info_field_an>			(dst, "AN");
		add_subfield <info_field_bq>			(dst, "BQ");
		add_subfield <info_field_cigar>			(dst, "CIGAR");
		add_subfield <info_field_db>			(dst, "DB");
		add_subfield <info_field_dp>			(dst, "DP");
		add_subfield <info_field_end>			(dst, "END");
		add_subfield <info_field_h2>			(dst, "H2");
		add_subfield <info_field_h3>			(dst, "H3");
		add_subfield <info_field_mq>			(dst, "MQ");
		add_subfield <info_field_mq0>			(dst, "MQ0");
		add_subfield <info_field_ns>			(dst, "NS");
		add_subfield <info_field_sb>			(dst, "SB");
		add_subfield <info_field_somatic>		(dst, "SOMATIC");
		add_subfield <info_field_validated>		(dst, "VALIDATED");
		add_subfield <info_field_1000g>			(dst, "1000G");
	}


	void add_reserved_genotype_keys(genotype_field_map &dst)
	{
		add_subfield <genotype_field_ad>	(dst, "AD");
		add_subfield <genotype_field_adf>	(dst, "ADF");
		add_subfield <genotype_field_adr>	(dst, "ADR");
		add_subfield <genotype_field_dp>	(dst, "DP");
		add_subfield <genotype_field_ec>	(dst, "EC");
		add_subfield <genotype_field_ft>	(dst, "FT");
		add_subfield <genotype_field_gl>	(dst, "GL");
		add_subfield <genotype_field_gp>	(dst, "GP");
		add_subfield <genotype_field_gq>	(dst, "GQ");
		add_subfield <genotype_field_gt>	(dst, "GT");
		add_subfield <genotype_field_hq>	(dst, "HQ");
		add_subfield <genotype_field_mq>	(dst, "MQ");
		add_subfield <genotype_field_pl>	(dst, "PL");
		add_subfield <genotype_field_pq>	(dst, "PQ");
		add_subfield <genotype_field_ps>	(dst, "PS");
	}
}

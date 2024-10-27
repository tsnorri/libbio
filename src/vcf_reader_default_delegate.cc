/*
 * Copyright (c) 2022-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <iostream>
#include <libbio/vcf/constants.hh>
#include <libbio/vcf/subfield/base.hh>
#include <libbio/vcf/vcf_reader.hh>
#include <string>

namespace vcf	= libbio::vcf;


namespace {

	void report_error(char const *field_type, std::string const &key, vcf::subfield_base const &field, vcf::metadata_formatted_field const &meta)
	{
		std::cerr
			<< "WARNING: The definition of the "
			<< field_type
			<< " field “"
			<< key
			<< "” in the VCF headers differs from that in the predefined fields (";
		vcf::output_vcf_value(std::cerr, meta.get_value_type());
		std::cerr << ", ";
		vcf::output_vcf_value(std::cerr, meta.get_number());
		std::cerr << " vs. ";
		vcf::output_vcf_value(std::cerr, field.metadata_value_type());
		std::cerr << ", ";
		vcf::output_vcf_value(std::cerr, field.number());
		std::cerr << ").\n";
	}
}


namespace libbio::vcf {

	bool reader_default_delegate::vcf_reader_should_replace_non_matching_subfield(
		std::string const &key,
		info_field_base const &field,
		metadata_info const &meta
	)
	{
		report_error("info", key, field, meta);
		return true;
	}

	bool reader_default_delegate::vcf_reader_should_replace_non_matching_subfield(
		std::string const &key,
		genotype_field_base const &field,
		metadata_format const &meta
	)
	{
		report_error("genotype", key, field, meta);
		return true;
	}
}

/*
 * Copyright (c) 2019-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <cstdint>
#include <libbio/vcf/constants.hh>
#include <libbio/vcf/metadata.hh>
#include <ostream>
#include <stdexcept>


namespace libbio::vcf {

	void metadata_formatted_field::check_field(std::int32_t const number, metadata_value_type const vt) const
	{
		switch (vt)
		{
			case metadata_value_type::UNKNOWN:
				throw std::runtime_error("Field contents were to be parsed but field value type was set to unknown");

			default:
			{
				if (vt != m_value_type)
					throw std::runtime_error("Value type mismatch");
				break;
			}
		}

		if (! (number == VCF_NUMBER_DETERMINED_AT_RUNTIME || number == m_number))
			throw std::runtime_error("Cardinality mismatch");
	}


	// FIXME: refine the output formats.

	void metadata_info::output_vcf(std::ostream &stream) const
	{
		stream << "##INFO=<ID=" << get_id() << ",Number=";
		output_vcf_value(stream, get_number());
		stream << ",Type=";
		output_vcf_value(stream, get_value_type());
		stream
			<< ",Description=\"" << get_description()
			<< "\",Source=\"" << get_source()
			<< "\",Version=\"" << get_version()
			<< "\">\n";
	}


	void metadata_format::output_vcf(std::ostream &stream) const
	{
		stream << "##FORMAT=<ID=" << get_id() << ",Number=";
		output_vcf_value(stream, get_number());
		stream << ",Type=";
		output_vcf_value(stream, get_value_type());
		stream << ",Description=\"" << get_description() << "\">\n";
	}


	void metadata_filter::output_vcf(std::ostream &stream) const
	{
		stream << "##FILTER=<ID=" << get_id() << ",Description=\"" << get_description() << "\">\n";
	}


	void metadata_alt::output_vcf(std::ostream &stream) const
	{
		stream << "##ALT=<ID=" << get_id() << ",Description=\"" << get_description() << "\">\n";
	}


	void metadata_assembly::output_vcf(std::ostream &stream) const
	{
		stream << "##assembly=" << get_assembly() << '\n';
	}


	void metadata_contig::output_vcf(std::ostream &stream) const
	{
		stream << "##contig=<ID=" << get_id() << ",length=" << get_length() << ">\n";
	}
}

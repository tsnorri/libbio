/*
 * Copyright (c) 2019 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <libbio/vcf/vcf_metadata.hh>


namespace libbio {

	// FIXME: refine the output formats.
	
	void vcf_metadata_info::output_vcf(std::ostream &stream) const
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
	
	
	bool vcf_metadata_info::output_vcf_field(std::ostream &stream, variant_base const &var, char const *sep) const
	{
		if (var.m_assigned_info_fields[m_index])
		{
			stream << sep << get_id();
			if (vcf_metadata_value_type::FLAG != get_value_type())
			{
				stream << '=';
				m_field->output_vcf_value(stream, var);
			}
			return true;
		}
		return false;
	}
	
	
	void vcf_metadata_format::output_vcf(std::ostream &stream) const
	{
		stream << "##FORMAT=<ID=" << get_id() << ",Number=";
		output_vcf_value(stream, get_number());
		stream << ",Type=";
		output_vcf_value(stream, get_value_type());
		stream << ",Description=\"" << get_description() << "\">\n";
	}
	
	
	bool vcf_metadata_format::output_vcf_field(std::ostream &stream, variant_sample const &sample, char const *sep) const
	{
		if (vcf_metadata_value_type::NOT_PROCESSED == m_field->value_type())
			return false;
		stream << sep;
		m_field->output_vcf_value(stream, sample);
		return true;
	}
	
	
	void vcf_metadata_filter::output_vcf(std::ostream &stream) const
	{
		stream << "##FILTER=<ID=" << get_id() << ",Description=\"" << get_description() << "\">\n";
	}
	
	
	void vcf_metadata_alt::output_vcf(std::ostream &stream) const
	{
		stream << "##ALT=<ID=" << get_id() << ",Description=\"" << get_description() << "\">\n";
	}
	
	
	void vcf_metadata_assembly::output_vcf(std::ostream &stream) const
	{
		stream << "##assembly=" << get_assembly() << '\n';
	}
	
	
	void vcf_metadata_contig::output_vcf(std::ostream &stream) const
	{
		stream << "##contig=<ID=" << get_id() << ",length=" << get_length() << ">\n";
	}
}

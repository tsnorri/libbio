/*
 * Copyright (c) 2017 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <libbio/types.hh>

namespace libbio {
	
	char const *to_string(sv_type const svt)
	{
		switch (svt)
		{
			case sv_type::NONE:
				return "(none)";
			
			case sv_type::DEL:
				return "DEL";
			
			case sv_type::INS:
				return "INS";
			
			case sv_type::DUP:
				return "DUP";
			
			case sv_type::INV:
				return "INV";
			
			case sv_type::CNV:
				return "CNV";
			
			case sv_type::DUP_TANDEM:
				return "DUP:TANDEM";
				
			case sv_type::DEL_ME:
				return "DEL:ME";
				
			case sv_type::INS_ME:
				return "INS:ME";
				
			case sv_type::UNKNOWN:
				return "(unknown)";
				
			default:
				return "(unexpected value)";
		}
	}
	
	
	void output_vcf_value(std::ostream &stream, std::int32_t const value)
	{
		switch (value)
		{
			case VCF_NUMBER_UNKNOWN:
				stream << '.';
				break;
			case VCF_NUMBER_ONE_PER_ALTERNATE_ALLELE:
				stream << 'A';
				break;
			case VCF_NUMBER_ONE_PER_ALLELE:
				stream << 'R';
				break;
			case VCF_NUMBER_ONE_PER_GENOTYPE:
				stream << 'G';
				break;
			default:
				stream << value;
				break;
		}
	}
	
	
	void output_vcf_value(std::ostream &os, vcf_metadata_value_type const vt)
	{
		switch (vt)
		{
			case UNKNOWN:
				os << '.';
				break;
			case NOT_PROCESSED:
				os << "(Not processed)";
				break;
			case INTEGER:
				os << "Integer";
				break;
			case FLOAT:
				os << "Float";
				break;
			case CHARACTER:
				os << "Character";
				break;
			case STRING:
				os << "String";
				break;
			case FLAG:
				os << "Flag";
				break;
			default:
				os << "(Unknown value type)";
				break;
		}
	}
}

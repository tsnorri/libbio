/*
 * Copyright (c) 2017-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

 #include <cstdint>
#include <libbio/vcf/constants.hh>
#include <ostream>


namespace libbio {

	char const *to_string(vcf::sv_type const svt)
	{
		switch (svt)
		{
			case vcf::sv_type::NONE:
				return "(none)";

			case vcf::sv_type::DEL:
				return "DEL";

			case vcf::sv_type::INS:
				return "INS";

			case vcf::sv_type::DUP:
				return "DUP";

			case vcf::sv_type::INV:
				return "INV";

			case vcf::sv_type::CNV:
				return "CNV";

			case vcf::sv_type::DUP_TANDEM:
				return "DUP:TANDEM";

			case vcf::sv_type::DEL_ME:
				return "DEL:ME";

			case vcf::sv_type::INS_ME:
				return "INS:ME";

			case vcf::sv_type::UNKNOWN_SV:
				return "(unknown structural variant)";

			case vcf::sv_type::UNKNOWN:
				return "(unknown ALT)";

			default:
				return "(unexpected value)";
		}
	}
}


namespace libbio::vcf {

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


	void output_vcf_value(std::ostream &os, metadata_value_type const vt)
	{
		switch (vt)
		{
			case metadata_value_type::UNKNOWN:
				os << '.';
				break;
			case metadata_value_type::NOT_PROCESSED:
				os << "(Not processed)";
				break;
			case metadata_value_type::INTEGER:
				os << "Integer";
				break;
			case metadata_value_type::FLOAT:
				os << "Float";
				break;
			case metadata_value_type::CHARACTER:
				os << "Character";
				break;
			case metadata_value_type::STRING:
				os << "String";
				break;
			case metadata_value_type::FLAG:
				os << "Flag";
				break;
			default:
				os << "(Unknown value type)";
				break;
		}
	}
}

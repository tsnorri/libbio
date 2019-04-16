/*
 * Copyright (c) 2017-2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_TYPES_HH
#define LIBBIO_TYPES_HH

#include <cstdint>
#include <limits>
#include <ostream>


namespace libbio {

	enum class vcf_field : std::uint8_t {
		CHROM	= 0,
		POS		= 1,
		ID		= 2,
		REF		= 3,
		ALT		= 4,
		QUAL	= 5,
		FILTER	= 6,
		INFO	= 7,
		FORMAT	= 8,
		ALL		= 9,
		VARY	= 10
	};
	
	enum class format_field : std::uint8_t {
		GT		= 0,
		DP,
		GQ,
		PS,
		PQ,
		MQ
	};
	
	enum { NULL_ALLELE = std::numeric_limits <std::uint8_t>::max() };
	
	enum class sv_type : std::uint8_t {
		NONE		= 0,
		DEL,
		INS,
		DUP,
		INV,
		CNV,
		DUP_TANDEM,
		DEL_ME,
		INS_ME,
		UNKNOWN
	};
	
	
	enum vcf_metadata_value_type : std::uint8_t
	{
		UNKNOWN = 0,
		NOT_PROCESSED,
		INTEGER,
		FLOAT,
		CHARACTER,
		STRING,
		FLAG
	};
	
	
	enum vcf_metadata_number
	{
		// VCF 4.3 specification, section 1.3 Data types: “For the Integer type, the values from −2^31 to −2^31 + 7 cannot be stored in the binary version and therefore are disallowed in both VCF and BCF”
		VCF_NUMBER_UNKNOWN					= INT32_MIN,		// .
		VCF_NUMBER_ONE_PER_ALTERNATE_ALLELE = INT32_MIN + 1,	// A
		VCF_NUMBER_ONE_PER_ALLELE			= INT32_MIN + 2,	// R
		VCF_NUMBER_ONE_PER_GENOTYPE			= INT32_MIN + 3,	// G
		VCF_NUMBER_DETERMINED_AT_RUNTIME	= INT32_MIN + 4
	};
	

	char const *to_string(sv_type const svt);
	
	void output_vcf_value(std::ostream &os, std::int32_t const);
	void output_vcf_value(std::ostream &os, vcf_metadata_value_type const);
}

#endif

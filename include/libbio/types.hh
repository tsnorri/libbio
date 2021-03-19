/*
 * Copyright (c) 2017-2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_TYPES_HH
#define LIBBIO_TYPES_HH

#include <cstdint>
#include <limits>
#include <ostream>


namespace libbio::vcf {

	enum class field : std::uint8_t {
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
		UNKNOWN_SV,
		UNKNOWN		// Denotes “.” ALT. FIXME: consider clarifying the name of the enum.
	};
	
	
	enum class metadata_type : std::uint8_t
	{
		UNKNOWN = 0,
		INFO,
		FORMAT,
		FILTER,
		ALT,
		ASSEMBLY,
		CONTIG
	};
	
	
	enum class metadata_value_type : std::uint8_t
	{
		UNKNOWN = 0,
		NOT_PROCESSED,
		INTEGER,
		FLOAT,
		CHARACTER,
		STRING,
		FLAG,
		// Special values.
		SCALAR_LIMIT,
		FIRST = INTEGER,
		VECTOR_LIMIT = FLAG
	};
	
	
	enum metadata_number : std::int32_t
	{
		// VCF 4.3 specification, section 1.3 Data types: “For the Integer type, the values from −2^31 to −2^31 + 7 cannot be stored in the binary version and therefore are disallowed in both VCF and BCF”
		VCF_NUMBER_UNKNOWN						= INT32_MIN,		// .
		VCF_NUMBER_ONE_PER_ALTERNATE_ALLELE	= INT32_MIN + 1,	// A
		VCF_NUMBER_ONE_PER_ALLELE				= INT32_MIN + 2,	// R
		VCF_NUMBER_ONE_PER_GENOTYPE			= INT32_MIN + 3,	// G
		VCF_NUMBER_DETERMINED_AT_RUNTIME		= INT32_MIN + 4,
		
		// Shorthand.
		VCF_NUMBER_A = VCF_NUMBER_ONE_PER_ALTERNATE_ALLELE,
		VCF_NUMBER_R = VCF_NUMBER_ONE_PER_ALLELE,
		VCF_NUMBER_G = VCF_NUMBER_ONE_PER_GENOTYPE
	};
	
	
	void output_vcf_value(std::ostream &os, std::int32_t const);
	void output_vcf_value(std::ostream &os, metadata_value_type const);
}


namespace libbio {

	char const *to_string(vcf::sv_type const svt);
}

#endif

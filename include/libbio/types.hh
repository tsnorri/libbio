/*
 * Copyright (c) 2017-2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_TYPES_HH
#define LIBBIO_TYPES_HH

#include <cstdint>
#include <limits>


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
	
	char const *to_string(sv_type const svt);
}

#endif

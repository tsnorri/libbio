/*
 * Copyright (c) 2023 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_SAM_RECORD_HH
#define LIBBIO_SAM_RECORD_HH

#include <cstdint>
#include <libbio/sam/cigar.hh>
#include <libbio/sam/optional_field.hh>
#include <string>
#include <vector>


namespace libbio::sam {
	
	typedef std::uint32_t	position_type;
	typedef std::uint8_t	mapping_quality_type;
	typedef std::uint32_t	reference_id_type;
	
	constexpr static inline reference_id_type const INVALID_REFERENCE_ID{std::numeric_limits <reference_id_type>::max()};
	
	
	struct record
	{
		std::string				qname;	// Empty for missing.
		std::vector <cigar_run>	cigar;
		std::vector <char>		seq;
		std::vector <char>		qual;
		optional_field			optional_fields;
		
		reference_id_type		rname_id{};
		reference_id_type		rnext_id{};
		
		position_type			pos{};
		position_type			pnext{};
		
		std::int32_t			tlen{};
		
		std::uint16_t			flag{};
		mapping_quality_type	mapq{};
	};
}

#endif

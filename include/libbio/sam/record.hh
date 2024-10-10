/*
 * Copyright (c) 2023-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_SAM_RECORD_HH
#define LIBBIO_SAM_RECORD_HH

#include <cstdint>
#include <libbio/sam/cigar.hh>
#include <libbio/sam/flag.hh>
#include <libbio/sam/optional_field.hh>
#include <string>
#include <vector>


namespace libbio::sam {
	
	typedef std::int32_t	position_type;
	typedef std::uint8_t	mapping_quality_type;
	typedef std::int32_t	reference_id_type;
	
	constexpr static inline position_type const INVALID_POSITION{-1};
	constexpr static inline reference_id_type const INVALID_REFERENCE_ID{-1};
	constexpr static inline mapping_quality_type const MAPQ_MIN{'!'};
	
	struct header; // Fwd.
	
	
	struct record
	{
		typedef std::vector <char>	sequence_type;
		typedef std::vector <char>	qual_type;
		
		std::string				qname;	// Empty for missing.
		std::vector <cigar_run>	cigar;
		sequence_type			seq;
		qual_type				qual;
		optional_field			optional_fields;
		
		reference_id_type		rname_id{};
		reference_id_type		rnext_id{};
		
		position_type			pos{};
		position_type			pnext{};
		
		std::int32_t			tlen{};
		std::uint16_t			bin{};
		
		flag_type				flag{};
		mapping_quality_type	mapq{MAPQ_MIN};
		
		constexpr bool is_primary() const { return 0 == std::to_underlying(flag & (flag::secondary_alignment | flag::supplementary_alignment)); } // SAMv1 § 1.4
		constexpr mapping_quality_type normalised_mapping_quality() const { return mapq - MAPQ_MIN; }
	};
	
	
	// Compares records.
	bool is_equal(header const &lhsh, header const &rhsh, record const &lhsr, record const &rhsr);
	// Ignore some type checks.
	bool is_equal_(header const &lhsh, header const &rhsh, record const &lhsr, record const &rhsr);
}

#endif

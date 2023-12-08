/*
 * Copyright (c) 2023 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_SAM_FLAG_HH
#define LIBBIO_SAM_FLAG_HH

#include <cstdint>


namespace libbio::sam {

	typedef std::uint16_t	flag_type;
	
	
	enum class flag : flag_type
	{
		template_has_multiple_segments	= 0x1,
		properly_aligned				= 0x2,
		unmapped						= 0x4,
		next_unmapped					= 0x8,
		reverse_complemented			= 0x10,
		next_reverse_complemented		= 0x20,
		first_segment					= 0x40,
		last_segment					= 0x80,
		secondary_alignment				= 0x100,
		failed_filter					= 0x200,
		duplicate						= 0x400,
		supplementary_alignment			= 0x800
	};
	
	
	constexpr inline flag operator|(flag const lhs, flag const rhs)
	{
		return static_cast <flag>(std::to_underlying(lhs) | std::to_underlying(rhs));
	}
	
	
	constexpr inline flag operator&(flag const lhs, flag const rhs)
	{
		return static_cast <flag>(std::to_underlying(lhs) & std::to_underlying(rhs));
	}
	
	
	constexpr inline flag &operator|=(flag &lhs, flag const rhs)
	{
		auto const res(lhs | rhs);
		lhs = res;
		return lhs;
	}
	
	
	constexpr inline flag &operator&=(flag &lhs, flag const rhs)
	{
		auto const res(lhs & rhs);
		lhs = res;
		return lhs;
	}
}

#endif

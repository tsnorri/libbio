/*
 * Copyright (c) 2023-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_SAM_FLAG_HH
#define LIBBIO_SAM_FLAG_HH

#include <cstdint>
#include <type_traits>
#include <utility>		// std::to_underlying


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
	
	
	template <typename t_type>
	concept Flag = std::is_same_v <t_type, flag_type> || std::is_same_v <t_type, flag>;
}


namespace libbio::sam::detail {
	
	template <Flag t_flag> constexpr flag_type flag_value(t_flag const flag) { return std::to_underlying(flag); }
	template <> constexpr flag_type flag_value(flag_type const flag) { return flag; }
}


namespace libbio::sam {

	constexpr inline flag operator|(flag const lhs, flag const rhs)
	{
		return static_cast <flag>(std::to_underlying(lhs) | std::to_underlying(rhs));
	}
	
	
	template <Flag t_lhs, Flag t_rhs>
	constexpr inline flag operator&(t_lhs const lhs, t_rhs const rhs)
	{
		return static_cast <flag>(detail::flag_value(lhs) & detail::flag_value(rhs));
	}
	
	
	constexpr inline flag &operator|=(flag &lhs, flag const rhs)
	{
		auto const res(lhs | rhs);
		lhs = res;
		return lhs;
	}
	
	
	template <Flag t_lhs, Flag t_rhs>
	constexpr inline flag &operator&=(t_lhs &lhs, t_rhs const rhs)
	{
		auto const res(lhs & rhs);
		lhs = res;
		return lhs;
	}
}

#endif

/*
 * Copyright (c) 2023 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_GENERIC_PARSER_FIELD_POSITION_HH
#define LIBBIO_GENERIC_PARSER_FIELD_POSITION_HH

#include <cstdint>
#include <utility> // std::to_underlying


namespace libbio::parsing {
	
	enum class field_position : std::uint8_t
	{
		none_		= 0x0,
		initial_	= 0x1,
		middle_		= 0x2,
		final_		= 0x4
	};
	
	
	constexpr inline field_position operator|(field_position const lhs, field_position const rhs)
	{
		return static_cast <field_position>(std::to_underlying(lhs) | std::to_underlying(rhs));
	}

	constexpr inline field_position &operator|=(field_position &lhs, field_position const rhs)
	{
		auto const res(lhs | rhs);
		lhs = res;
		return lhs;
	}
	
	constexpr inline field_position operator&(field_position const lhs, field_position const rhs)
	{
		return static_cast <field_position>(std::to_underlying(lhs) & std::to_underlying(rhs));
	}
	
	constexpr inline bool any(field_position const fp)
	{
		return std::to_underlying(fp) ? true : false;
	}
}

#endif

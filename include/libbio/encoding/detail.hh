/*
 * Copyright (c) 2026 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_ENCODING_DETAIL_HH
#define LIBBIO_ENCODING_DETAIL_HH

#include <concepts>
#include <type_traits>


namespace libbio::encoding::detail {

	template <std::unsigned_integral t_value, std::make_signed_t <t_value> t_constant>
	constexpr inline t_value add_signed_constant_to_unsigned(t_value value)
	{
		if constexpr (0 <= t_constant)
			return value + t_constant;
		else
			return value - t_value{-t_constant};
	}
}

#endif

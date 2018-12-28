/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_TYPE_TRAITS_HH
#define LIBBIO_TYPE_TRAITS_HH

#include <type_traits>


namespace libbio {

	template <typename t_type>
	inline constexpr bool is_integral_rr_v = std::is_integral_v <std::remove_reference_t <t_type>>;

	template <typename t_type>
	inline constexpr bool is_arithmetic_rr_v = std::is_arithmetic_v <std::remove_reference_t <t_type>>;
	
	template <typename t_type>
	inline constexpr bool is_signed_rr_v = std::is_signed_v <std::remove_reference_t <t_type>>;

	template <typename t_type>
	inline constexpr bool is_unsigned_rr_v = std::is_unsigned_v <std::remove_reference_t <t_type>>;
	
	template <typename t_type>
	using make_unsigned_rr_t = typename std::make_unsigned_t <std::remove_reference_t <t_type>>;
}

#endif

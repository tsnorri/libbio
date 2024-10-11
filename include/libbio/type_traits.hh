/*
 * Copyright (c) 2018-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_TYPE_TRAITS_HH
#define LIBBIO_TYPE_TRAITS_HH

#include <type_traits>


namespace libbio {
	
	template <typename t_base, typename t_class>
	constexpr static bool const is_base_of_rcvr_v{std::is_base_of_v <t_base, std::remove_cvref_t <t_class>>};
	
	template <typename t_type>
	constexpr static bool const is_integral_rcvr_v{std::is_integral_v <std::remove_cvref_t <t_type>>};
	
	template <typename t_type>
	constexpr static bool const is_floating_point_rcvr_v{std::is_floating_point_v <std::remove_cvref_t <t_type>>};
	
	template <typename t_type>
	constexpr static bool const is_trivial_rcvr_v{std::is_trivial_v <std::remove_cvref_t <t_type>>};
	
	template <typename t_type>
	constexpr static bool const is_integral_rr_v = std::is_integral_v <std::remove_reference_t <t_type>>;

	template <typename t_type>
	constexpr static bool const is_arithmetic_rr_v = std::is_arithmetic_v <std::remove_reference_t <t_type>>;
	
	template <typename t_type>
	constexpr static bool const is_signed_rr_v = std::is_signed_v <std::remove_reference_t <t_type>>;

	template <typename t_type>
	constexpr static bool const is_unsigned_rr_v = std::is_unsigned_v <std::remove_reference_t <t_type>>;
	
	
	template <typename t_type>
	using make_unsigned_rr_t = typename std::make_unsigned_t <std::remove_reference_t <t_type>>;
	
	template <typename t_type, typename t_true, typename t_false>
	using if_const_t = std::conditional_t <
		std::is_const_v <std::remove_reference_t <t_type>>,
		t_true,
		t_false
	>;
}

#endif

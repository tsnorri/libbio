/*
 * Copyright (c) 2018-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_UTILITY_IS_EQUAL_HH
#define LIBBIO_UTILITY_IS_EQUAL_HH

#include <libbio/type_traits.hh>
#include <type_traits>


namespace libbio { namespace detail {
	
	template <typename t_lhs, typename t_rhs, bool t_lhs_signed = is_signed_rr_v <t_lhs>, bool t_rhs_signed = is_signed_rr_v <t_rhs>>
	struct equal_integral // Both signed or unsigned b.c. of the specializations below.
	{
		static_assert(is_integral_rr_v <t_lhs>);
		static_assert(is_integral_rr_v <t_rhs>);
		
		constexpr static inline bool check(t_lhs &&lhs, t_rhs &&rhs) { return lhs == rhs; }
	};
	
	
	template <typename t_lhs, typename t_rhs>
	struct equal_integral <t_lhs, t_rhs, true, false> // lhs is signed, rhs unsigned.
	{
		static_assert(is_integral_rr_v <t_lhs>);
		static_assert(is_integral_rr_v <t_rhs>);
		
		constexpr static inline bool check(t_lhs &&lhs, t_rhs &&rhs) { return (0 <= lhs) && (make_unsigned_rr_t <t_lhs>(lhs) == rhs); }
	};
	
	
	template <typename t_lhs, typename t_rhs>
	struct equal_integral <t_lhs, t_rhs, false, true> // lhs is unsigned, rhs signed.
	{
		static_assert(is_integral_rr_v <t_lhs>);
		static_assert(is_integral_rr_v <t_rhs>);
		
		constexpr static inline bool check(t_lhs &&lhs, t_rhs &&rhs) { return (0 <= rhs) && (lhs == make_unsigned_rr_t <t_rhs>(rhs)); }
	};
	
	
	template <bool t_expected = true>
	struct equal
	{
		template <typename t_lhs, typename t_rhs>
		constexpr static bool check(t_lhs &&lhs, t_rhs &&rhs)
		{
			if constexpr (is_integral_rr_v <t_lhs> && is_integral_rr_v <t_rhs>)
				return (!t_expected) ^ equal_integral <t_lhs, t_rhs>::check(std::forward <t_lhs>(lhs), std::forward <t_rhs>(rhs));
			else
				return (!t_expected) ^ (lhs == rhs);
		}

		template <typename t_lhs, typename t_rhs>
		constexpr bool operator()(t_lhs &&lhs, t_rhs &&rhs) const { return check(std::forward <t_lhs>(lhs), std::forward <t_rhs>(rhs)); }
	};
}}


namespace libbio {

	template <typename t_lhs, typename t_rhs>
	constexpr inline bool is_equal(t_lhs &&lhs, t_rhs &&rhs) { return detail::equal <>::check(std::forward <t_lhs>(lhs), std::forward <t_rhs>(rhs)); }

	typedef detail::equal <>		is_equal_;
	typedef detail::equal <false>	is_not_equal_;
}

#endif

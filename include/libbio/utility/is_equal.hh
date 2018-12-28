/*
 * Copyright (c) 2018 Tuukka Norri
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
		
		static inline bool check(t_lhs &&lhs, t_rhs &&rhs) { return lhs == rhs; }
	};
	
	
	template <typename t_lhs, typename t_rhs>
	struct equal_integral <t_lhs, t_rhs, true, false> // lhs is signed, rhs unsigned.
	{
		static_assert(is_integral_rr_v <t_lhs>);
		static_assert(is_integral_rr_v <t_rhs>);
		
		static inline bool check(t_lhs &&lhs, t_rhs &&rhs) { return (0 <= lhs) && (make_unsigned_rr_t <t_lhs>(lhs) == rhs); }
	};
	
	
	template <typename t_lhs, typename t_rhs>
	struct equal_integral <t_lhs, t_rhs, false, true> // lhs is unsigned, rhs signed.
	{
		static_assert(is_integral_rr_v <t_lhs>);
		static_assert(is_integral_rr_v <t_rhs>);
		
		static inline bool check(t_lhs &&lhs, t_rhs &&rhs) { return (0 <= rhs) && (lhs == make_unsigned_rr_t <t_rhs>(rhs)); }
	};
	
	
	template <typename t_lhs, typename t_rhs>
	struct equal
	{
		static inline bool check(t_lhs &&lhs, t_rhs &&rhs)
		{
			if constexpr (is_integral_rr_v <t_lhs> && is_integral_rr_v <t_rhs>)
				return equal_integral <t_lhs, t_rhs>::check(std::forward <t_lhs>(lhs), std::forward <t_rhs>(rhs));
			else
				return lhs == rhs;
		}
	};
}}


namespace libbio {

	template <typename t_lhs, typename t_rhs>
	inline bool is_equal(t_lhs &&lhs, t_rhs &&rhs) { return detail::equal <t_lhs, t_rhs>::check(std::forward <t_lhs>(lhs), std::forward <t_rhs>(rhs)); }
}

#endif

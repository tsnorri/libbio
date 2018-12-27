/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_UTILITY_IS_LTE_HH
#define LIBBIO_UTILITY_IS_LTE_HH

#include <libbio/type_traits.hh>


namespace libbio { namespace detail {
	
	template <typename t_lhs, typename t_rhs, bool t_lhs_signed = std::is_signed_v <t_lhs>, bool t_rhs_signed = std::is_signed_v <t_rhs>>
	struct lte_integral // Both signed or unsigned b.c. of the specializations below.
	{
		static_assert(is_integral_ref_v <t_lhs>);
		static_assert(is_integral_ref_v <t_rhs>);
		
		static inline bool check(t_lhs &&lhs, t_rhs &&rhs) { return lhs <= rhs; }
	};
	
	
	template <typename t_lhs, typename t_rhs>
	struct lte_integral <t_lhs, t_rhs, true, false> // lhs is signed, rhs unsigned.
	{
		static_assert(is_integral_ref_v <t_lhs>);
		static_assert(is_integral_ref_v <t_rhs>);
		
		static inline bool check(t_lhs &&lhs, t_rhs &&rhs) { return lhs < 0 || static_cast <t_rhs>(lhs) <= rhs; }
	};
	
	
	template <typename t_lhs, typename t_rhs>
	struct lte_integral <t_lhs, t_rhs, false, true> // lhs is unsigned, rhs signed.
	{
		static_assert(is_integral_ref_v <t_lhs>);
		static_assert(is_integral_ref_v <t_rhs>);
		
		static inline bool check(t_lhs &&lhs, t_rhs &&rhs) { return 0 < rhs && lhs <= static_cast <t_lhs>(rhs); }
	};
	
	
	template <typename t_lhs, typename t_rhs>
	struct lte
	{
		static inline bool check(t_lhs &&lhs, t_rhs &&rhs)
		{
			if constexpr (is_integral_ref_v <t_lhs> && is_integral_ref_v <t_rhs>)
				return lte_integral <t_lhs, t_rhs>::check(std::forward <t_lhs>(lhs), std::forward <t_rhs>(rhs));
			else
				return lhs <= rhs;
		}
	};
}}


namespace libbio {

	template <typename t_lhs, typename t_rhs>
	inline bool is_lte(t_lhs &&lhs, t_rhs &&rhs) { return detail::lte <t_lhs, t_rhs>::check(std::forward <t_lhs>(lhs), std::forward <t_rhs>(rhs)); }
}

#endif

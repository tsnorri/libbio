/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_UTIL_HH
#define LIBBIO_UTIL_HH

#include <atomic>


namespace libbio { namespace detail {
	
	template <typename t_type, typename t_first, typename... t_rest>
	struct smallest_unsigned_lockfree_type_gte_helper
	{
		typedef std::conditional_t <
			sizeof(t_first) <= sizeof(t_type) && std::atomic <t_type>::is_always_lock_free(),
			t_first,
			typename smallest_unsigned_lockfree_type_gte_helper <t_type, t_rest...>::type
		> type;
	};
}}


namespace libbio {
	
	// Make a type const conditionally.
	template <typename t_type, bool t_make_const>
	struct make_const
	{
		typedef t_type type;
	};
	
	template <typename t_type>
	struct make_const <t_type, true>
	{
		typedef t_type const type;
	};
	
	template <typename t_type, bool t_make_const>
	using make_const_t = typename make_const <t_type, t_make_const>::type;
	
	
	// Comparison helper.
	template <typename t_lhs, typename t_rhs>
	struct compare_lhs_first_lt
	{
		bool operator()(t_lhs const &lhs, t_rhs const &rhs) const
		{
			return lhs.first < rhs;
		}
	};
	
	
	template <typename t_type>
	struct smallest_unsigned_lockfree_type_gte
	{
		typedef typename detail::smallest_unsigned_lockfree_type_gte_helper <
			t_type, uint8_t, uint16_t, uint32_t, uint64_t
		>::type type;
	};
	
	template <typename t_type>
	using smallest_unsigned_lockfree_type_gte_t = typename smallest_unsigned_lockfree_type_gte <t_type>::type;
}

#endif

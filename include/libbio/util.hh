/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_UTIL_HH
#define LIBBIO_UTIL_HH


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
}

#endif

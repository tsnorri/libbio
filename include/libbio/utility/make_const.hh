/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_UTILITY_MAKE_CONST_HH
#define LIBBIO_UTILITY_MAKE_CONST_HH


namespace libbio { namespace detail {
	
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
}}


namespace libbio {
	
	template <typename t_type, bool t_make_const>
	using make_const_t = typename detail::make_const <t_type, t_make_const>::type;
}

#endif

/*
 * Copyright (c) 2022 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_TUPLE_UNIQUE_HH
#define LIBBIO_TUPLE_UNIQUE_HH

#include <libbio/tuple/find.hh>
#include <libbio/tuple/fold.hh>
#include <libbio/tuple/utility.hh>	// libbio::is_tuple_v


namespace libbio::tuples {
	
	// Erases duplicate types from t_tuple.
	// Could be more efficient if implemented as right fold (but tuples are not typically very big).
	template <typename t_tuple>
	requires is_tuple_v <t_tuple>
	struct unique
	{
		template <typename t_acc, typename t_type>
		using fold_fn = std::conditional_t <
			find <t_acc, t_type>::found,
			t_acc,
			append_t <t_acc, t_type>
		>;
		
		typedef foldl_t <fold_fn, empty, t_tuple>	type;
	};
	
	
	template <typename t_tuple>
	requires is_tuple_v <t_tuple>
	using unique_t = typename unique <t_tuple>::type;
}

#endif

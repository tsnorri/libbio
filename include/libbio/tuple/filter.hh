/*
 * Copyright (c) 2022 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_TUPLE_FILTER_HH
#define LIBBIO_TUPLE_FILTER_HH

#include <libbio/tuple/fold.hh>


namespace libbio::tuples {
	
	// Filter == keep.
	template <typename t_tuple, template <typename...> typename t_fn>
	struct filter
	{
		template <typename t_acc, typename t_type>
		using fold_fn = std::conditional_t <
			t_fn <t_type>::value,
			append_t <t_acc, t_type>,
			t_acc
		>;
		
		typedef foldl_t <fold_fn, empty, t_tuple>	type;
	};
	
	
	template <typename t_tuple, template <typename...> typename t_fn>
	using filter_t = typename filter <t_tuple, t_fn>::type;
}

#endif

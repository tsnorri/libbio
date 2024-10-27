/*
 * Copyright (c) 2022-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_TUPLE_FOLD_HH
#define LIBBIO_TUPLE_FOLD_HH

#include <libbio/tuple/slice.hh>
#include <libbio/tuple/utility.hh>	// libbio::is_tuple_v
#include <tuple>
#include <type_traits>


namespace libbio::tuples::detail {

	// By wrapping t_type we can use std::invoke_result_t on a helper function without accidentally
	// evaluating the (possibly missing or not accessible) constructor of t_type.
	template <typename t_type>
	struct wrapper
	{
		typedef t_type	wrapped_type;
	};
}


namespace libbio::tuples {

	// Left fold for tuples, i.e.
	// Tuple f => (X -> b -> Y) -> a -> f b -> c
	// s.t. X = a on the first iteration and Y = c on the last.
	template <template <typename...> typename, typename, typename t_tuple>
	requires is_tuple_v <t_tuple>
	struct foldl {};


	template <template <typename...> typename t_fn, typename t_acc, typename... t_args>
	struct foldl <t_fn, t_acc, std::tuple <t_args...>>
	{
		consteval static auto helper()
		{
			typedef std::tuple <t_args...> input_tuple_type;
			if constexpr (0 == std::tuple_size_v <input_tuple_type>)
				return detail::wrapper <t_acc>{};
			else
			{
				typedef std::tuple_element_t <0, input_tuple_type>	head_type;
				typedef slice_t <input_tuple_type, 1>				tail_type;
				typedef t_fn <t_acc, head_type>						result_type;
				return foldl <t_fn, result_type, tail_type>::helper();
			}
		}

		typedef typename std::invoke_result_t <decltype(&foldl::helper)>::wrapped_type type;
	};


	template <template <typename...> typename t_fn, typename t_acc, typename t_tuple>
	using foldl_t = typename foldl <t_fn, t_acc, t_tuple>::type;
}

#endif

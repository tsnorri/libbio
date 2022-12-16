/*
 * Copyright (c) 2022 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_TUPLE_FOLD_HH
#define LIBBIO_TUPLE_FOLD_HH

#include <libbio/tuple/slice.hh>


namespace libbio::tuples::detail {
	
	template <typename>
	struct visit_parameters {};
	
	
	template <typename... t_args>
	struct visit_parameters <std::tuple <t_args...>>
	{
		template <typename t_fn>
		consteval static decltype(auto) call(t_fn &&fn)
		{
			return fn.template operator() <t_args...>();
		}
	};
}


namespace libbio::tuples {
	
	// Left fold for tuples, i.e.
	// Tuple f => (X -> b -> Y) -> a -> f b -> c
	// s.t. X = a on the first iteration and Y = c on the last.
	template <template <typename...> typename, typename, typename>
	struct foldl
	{
		typedef struct cannot_fold_because_given_type_is_not_std_tuple {} type;
	};
	
	
	template <template <typename...> typename t_fn, typename t_acc, typename... t_args>
	struct foldl <t_fn, t_acc, std::tuple <t_args...>>
	{
		consteval static auto helper()
		{
			typedef std::tuple <t_args...> input_tuple_type;
			if constexpr (0 == std::tuple_size_v <input_tuple_type>)
			{
				// FIXME: try std::declval?
				return t_acc{};
			}
			else
			{
				typedef std::tuple_element_t <0, input_tuple_type>	head_type;
				typedef slice_t <input_tuple_type, 1>				tail_type;
				typedef t_fn <t_acc, head_type>						result_type;
				return foldl <t_fn, result_type, tail_type>::helper();
			}
		}
		
		typedef std::invoke_result_t <decltype(&foldl::helper)> type;
	};
	
	
	template <template <typename...> typename t_fn, typename t_acc, typename t_tuple>
	using foldl_t = typename foldl <t_fn, t_acc, t_tuple>::type;
	
	
	// Access the parameters for use with e.g. a fold expression.
	template <typename t_tuple, typename t_fn>
	consteval inline decltype(auto) visit_parameters(t_fn &&fn)
	{
		return detail::visit_parameters <t_tuple>::call(fn);
	}
}

#endif

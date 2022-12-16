/*
 * Copyright (c) 2022 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_TUPLE_ERASE_HH
#define LIBBIO_TUPLE_ERASE_HH

#include <libbio/tuple/cat.hh>
#include <type_traits>

namespace libbio::tuples::detail {
	
	template <typename t_type>
	struct is_same_as
	{
		template <typename t_other_type>
		struct type
		{
			constexpr static inline bool const value{std::is_same_v <t_type, t_other_type>};
		};
	};
}



namespace libbio::tuples {
	
	// Erases types that satisfy the given predicate from the given tuple.
	// Variable templates cannot be passed as template parameters as of C++20, hence the use of class template.
	template <typename t_tuple, template <typename> typename t_predicate>
	struct erase_if
	{
		template <std::size_t t_i>
		consteval static auto helper()
		{
			if constexpr (t_i == std::tuple_size_v <t_tuple>)
				return std::tuple <>{};
			else
			{
				typedef std::tuple_element_t <t_i, t_tuple> element_type;
				if constexpr (t_predicate <element_type>::value)
					return helper <1 + t_i>();
				else
					return std::tuple_cat(std::tuple <element_type>{}, helper <1 + t_i>());
			}
		}
		
		typedef std::invoke_result_t <decltype(&erase_if::helper <0>)> type;
	};
	
	
	template <typename t_tuple, template <typename> typename t_predicate>
	using erase_if_t = typename erase_if <t_tuple, t_predicate>::type;
	
	
	template <typename t_tuple, typename t_type>
	using erase_t = erase_if_t <t_tuple, detail::is_same_as <t_type>::template type>;
}

#endif

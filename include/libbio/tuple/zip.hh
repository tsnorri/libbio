/*
 * Copyright (c) 2025 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_TUPLE_ZIP_HH
#define LIBBIO_TUPLE_ZIP_HH

#include <cstddef>
#include <libbio/tuple/access.hh>
#include <libbio/tuple/map.hh>
#include <tuple>


namespace libbio::tuples::detail {

	template <typename... t_tuples>
	struct elements
	{
		template <typename t_index>
		using at_index = std::tuple <std::tuple_element_t <t_index::value, t_tuples>...>;

		template <typename t_index>
		using at_index_ = std::tuple <t_index, std::tuple_element_t <t_index::value, t_tuples>...>;
	};
}


namespace libbio::tuples {

	template <typename t_first, typename... t_rest>
	struct zip
	{
		static_assert(((size_v <t_first> == size_v <t_rest>) && ...), "All tuples must have the same size.");

		typedef map_t <index_constant_sequence_for_tuple <t_first>, detail::elements <t_first, t_rest...>::template at_index> type;
		typedef map_t <index_constant_sequence_for_tuple <t_first>, detail::elements <t_first, t_rest...>::template at_index_> indexed_elements_type;
	};

	template <typename... t_tuples>
	using zip_t = zip <t_tuples...>::type;

	template <typename... t_tuples>
	using indexed_elements_t = zip <t_tuples...>::indexed_elements_type;

	template <template <typename...> typename t_fn, typename... t_tuples>
	using zip_with_t = map_t <zip_t <t_tuples...>, t_fn>;
}

#endif

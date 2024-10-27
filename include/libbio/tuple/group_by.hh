/*
 * Copyright (c) 2022-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_TUPLE_GROUP_BY_HH
#define LIBBIO_TUPLE_GROUP_BY_HH

#include <libbio/tuple/access.hh>
#include <libbio/tuple/cat.hh>
#include <libbio/tuple/find.hh>
#include <libbio/tuple/fold.hh>
#include <libbio/tuple/map.hh>
#include <type_traits>				// std::bool_constant
#include <tuple>


namespace libbio::tuples {

	template <typename t_tuple, template <typename> typename t_callback>
	struct group_by_type
	{
		template <typename t_type>
		struct eq
		{
			template <typename t_tuple_>
			struct with : public std::bool_constant <std::is_same_v <t_callback <t_type>, head_t <t_tuple_>>> {};
		};

		// Add to the second item of the pair.
		template <typename t_pair, typename t_item>
		using modify_t = std::tuple <head_t <t_pair>, append_t <second_t <t_pair>, t_item>>;

		// Produce pairs (i.e. tuples of two items) s.t. the first is the result type of the callback and the second is a tuple of the types.
		template <typename t_acc, typename t_type, typename t_find_result =
			tuples::find_if <
				t_acc,
				eq <t_type>::template with,
				std::tuple <t_callback <t_type>, std::tuple <>>	// New group if no matches are found.
			>
		>
		using fold_callback_t = append_t <
			typename t_find_result::mismatches_type,
			modify_t <
				typename t_find_result::first_match_type,
				t_type
			>
		>;

		template <typename t_type>
		using map_callback_t = second_t <t_type>;

		typedef foldl_t <fold_callback_t, empty, t_tuple>	keyed_type;	// Pairs (tuples) of keys as determined by t_callback and tuples of values from t_tuple.
		typedef map_t <keyed_type, map_callback_t>			type;		// The second items of the pairs in keyed_type.
	};


	template <typename t_tuple, template <typename> typename t_callback>
	using group_by_type_t = typename group_by_type <t_tuple, t_callback>::type;
}

#endif

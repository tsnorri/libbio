/*
 * Copyright (c) 2022-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_TUPLE_FIND_HH
#define LIBBIO_TUPLE_FIND_HH

#include <cstddef>
#include <cstdint>
#include <libbio/tuple/access.hh>
#include <libbio/tuple/cat.hh>
#include <libbio/tuple/fold.hh>
#include <libbio/tuple/utility.hh>
#include <libbio/utility.hh>
#include <type_traits>


namespace libbio::tuples {

	struct not_found {};

	template <typename t_tuple, template <typename> typename t_predicate, typename t_default = not_found>
	requires is_tuple_v <t_tuple>
	struct find_if
	{
		template <
			std::size_t t_idx = 0,
			typename t_matches = empty,
			typename t_mismatches = empty,
			typename t_matching_indices = empty
		>
		struct accumulator
		{
			template <typename t_type>
			using next_matching_t = accumulator <1 + t_idx, append_t <t_matches, t_type>, t_mismatches, append_t <t_matching_indices, size_constant <t_idx>>>;

			template <typename t_type>
			using next_mismatching_t = accumulator <1 + t_idx, t_matches, append_t <t_mismatches, t_type>, t_matching_indices>;

			typedef t_matches			matches_type;
			typedef t_mismatches		mismatches_type;
			typedef t_matching_indices	matching_indices_type;
		};

		template <typename t_acc, typename t_type>
		using callback_t = std::conditional_t <
			t_predicate <t_type>::value,							// Check the predicate.
			typename t_acc::template next_matching_t <t_type>,
			typename t_acc::template next_mismatching_t <t_type>
		>;

		typedef foldl_t <callback_t, accumulator <>, t_tuple>	type_;
		typedef typename type_::matches_type					matches_type;
		typedef typename type_::matching_indices_type			matching_indices_type;
		typedef typename type_::mismatches_type					mismatches_type;
		typedef head_t_ <matches_type, t_default>				first_match_type;
		constexpr static inline bool const found{0 < std::tuple_size_v <matches_type>};
		constexpr static inline std::size_t const first_matching_index{head_t_ <matching_indices_type, size_constant <SIZE_MAX>>::value};
	};


	template <typename t_tuple, typename t_item, typename t_default = not_found>
	requires is_tuple_v <t_tuple>
	using find = find_if <t_tuple, same_as <t_item>::template with, t_default>;
}


namespace libbio::tuples::detail {
	template <typename t_tuple, typename t_item>
	struct first_index_of : public std::integral_constant <std::size_t, 0> {};

	template <template <typename...> typename t_tuple, typename ... t_args, typename t_item>
	requires is_tuple_v <t_tuple <t_item, t_args...>>
	struct first_index_of <t_tuple <t_item, t_args...>, t_item> : public std::integral_constant <std::size_t, 0> {};

	template <template <typename...> typename t_tuple, typename t_first, typename ... t_args, typename t_item>
	requires is_tuple_v <t_tuple <t_first, t_args...>>
	struct first_index_of <t_tuple <t_first, t_args...>, t_item>
	{
		constexpr static inline std::size_t const value{1 + first_index_of <t_tuple <t_args...>, t_item>::value};
	};
}


namespace libbio::tuples{

	template <
		typename t_tuple,
		typename t_item,
		bool t_should_assert_found = true,
		typename t_impl = detail::first_index_of <t_tuple, t_item>
	>
	requires is_tuple_v <t_tuple>
	struct first_index_of : std::integral_constant <std::size_t, (std::tuple_size_v <t_tuple> == t_impl::value ? SIZE_MAX : t_impl::value)>
	{
		static_assert(!(t_should_assert_found && std::tuple_size_v <t_tuple> == t_impl::value));
	};

	template <typename t_tuple, typename t_item, bool t_should_assert_found = true>
	constexpr static inline auto const first_index_of_v{first_index_of <t_tuple, t_item, t_should_assert_found>::value};
}

#endif

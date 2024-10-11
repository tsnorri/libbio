/*
 * Copyright (c) 2022-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_TUPLE_UTILITY_HH
#define LIBBIO_TUPLE_UTILITY_HH

#include <tuple>
#include <type_traits> // std::bool_constnat, std::false_type, std::integral_constant, std::true_type


namespace libbio::tuples {
	
	template <template <typename...> typename t_fn>
	struct negation
	{
		template <typename... t_args>
		struct with : std::bool_constant <!t_fn <t_args...>::value> {};
	};
	
	
	template <typename t_type>
	struct same_as
	{
		template <typename t_other_type>
		struct with : std::bool_constant <std::is_same_v <t_type, t_other_type>> {};
	};
	
	
	template <typename t_type>
	struct alignment : public std::integral_constant <std::size_t, alignof(t_type)> {};
	
	template <typename t_type>
	constexpr static inline auto const alignment_v{alignment <t_type>::value};
	
	
	template <typename t_type>
	struct is_tuple : public std::false_type {};
	
	template <typename... t_args>
	struct is_tuple <std::tuple <t_args...>> : public std::true_type {};
	
	template <typename t_type>
	constexpr static inline auto const is_tuple_v{is_tuple <t_type>::value};
	
	
	template <typename t_type>
	struct is_index_sequence : public std::false_type {};
	
	template <std::size_t... t_idxs>
	struct is_index_sequence <std::index_sequence <t_idxs...>> : public std::true_type {};
	
	template <typename t_type>
	constexpr static inline auto const is_index_sequence_v{is_index_sequence <t_type>::value};
}

#endif

/*
 * Copyright (c) 2022 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_FMAP_HH
#define LIBBIO_FMAP_HH

#include <tuple>
#include <utility>


namespace libbio::detail {
	
	template <std::size_t... t_indices, typename... t_args, typename t_fn>
	constexpr auto fmap(std::index_sequence <t_indices...> const &&, std::tuple <t_args...> &&args, t_fn &&fn)
	{
		return std::make_tuple(
			fn(
				std::forward <t_args>(
					std::get <t_indices>(args)
				)
			)...
		);
	}
}


namespace libbio {
	
	// Functional mapping for tuples, i.e. Functor f => f a -> (a -> b) -> f b
	// where f is std::tuple.
	template <typename t_type, typename t_fn>
	constexpr auto fmap(t_type &&value, t_fn &&fn)
	{
		return detail::fmap(
			std::make_index_sequence <std::tuple_size_v <t_type>>{},
			std::forward <t_type>(value),
			fn
		);
	}
	
	// Map an std::integer_sequence to an std::tuple.
	template <typename t_integer, std::size_t... t_indices, typename t_fn>
	constexpr auto map_to_tuple(std::integer_sequence <t_integer, t_indices...> &&indices, t_fn &&fn)
	{
		return std::make_tuple(fn(std::integral_constant <t_integer, t_indices>{})...);
	}
	
	// Return std::array instead.
	template <typename t_integer, std::size_t... t_indices, typename t_fn>
	constexpr auto map_to_array(std::integer_sequence <t_integer, t_indices...> &&indices, t_fn &&fn)
	{
		auto to_array([](auto && ... values){ return std::array{std::forward <decltype(values)>(values)...}; });
		return std::apply(to_array, std::make_tuple(fn(std::integral_constant <t_integer, t_indices>{})...));
	}
}

#endif

/*
 * Copyright (c) 2022 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_TUPLE_UTILITY_HH
#define LIBBIO_TUPLE_UTILITY_HH

#include <type_traits>


namespace libbio::tuples {
	
	template <template <typename...> typename t_fn>
	struct negation
	{
		template <typename... t_args>
		struct with
		{
			constexpr static inline bool value{!t_fn <t_args...>::value};
		};
	};
	
	
	template <typename t_type>
	struct same_as
	{
		template <typename t_other_type>
		struct with
		{
			constexpr static inline bool const value{std::is_same_v <t_type, t_other_type>};
		};
	};
	
	
	template <typename t_type>
	struct alignment
	{
		constexpr static inline std::size_t const value{alignof(t_type)};
	};
	
	template <typename t_type>
	constexpr static inline std::size_t const alignment_v{alignment <t_type>::value};
	
	
	template <typename t_type>
	struct is_tuple
	{
		constexpr static inline bool const value{false};
	};
	
	template <typename... t_args>
	struct is_tuple <std::tuple <t_args...>>
	{
		constexpr static inline bool const value{true};
	};
	
	template <typename t_type>
	constexpr static inline bool const is_tuple_v{is_tuple <t_type>::value};
}

#endif

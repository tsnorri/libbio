/*
 * Copyright (c) 2022-2023 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_GENERIC_PARSER_DELIMITER_HH
#define LIBBIO_GENERIC_PARSER_DELIMITER_HH

#include <cstdint>
#include <limits>		// std::numeric_limits
#include <type_traits>	// std::is_same_v


namespace libbio::parsing {
	
	typedef std::uint8_t delimiter_index_type; // Currently no need for more than 255 delimiters.
	constexpr static inline delimiter_index_type const INVALID_DELIMITER_INDEX{std::numeric_limits <delimiter_index_type>::max()};
	

	template <typename t_type>
	struct delimiter_type
	{
		typedef t_type	type;
		
		t_type value{};
		
		constexpr delimiter_type(t_type const value_):
			 value(value_)
		{
		}
		
		constexpr /* implicit */ operator t_type() const { return value; }
	};
}


namespace libbio::parsing::detail {

	template <delimiter_type t_first, delimiter_type... t_rest>
	requires ((std::is_same_v <decltype(t_first), decltype(t_rest)>) && ...)
	struct first_delimiter_type
	{
		typedef decltype(t_first)	type;
	};
	
	template <delimiter_type... t_delimiters>
	using first_delimiter_type_t = typename first_delimiter_type <t_delimiters...>::type;
	
	
	template <delimiter_type, delimiter_index_type, delimiter_type... t_rest> struct index_of {};
	
	template <delimiter_type t_delimiter, delimiter_index_type t_i>
	struct index_of <t_delimiter, t_i>
	{
		constexpr static inline delimiter_index_type const value{INVALID_DELIMITER_INDEX};
	};
	
	template <delimiter_type t_delimiter, delimiter_index_type t_i, delimiter_type t_first, delimiter_type... t_rest>
	struct index_of <t_delimiter, t_i, t_first, t_rest...>
	{
		constexpr static inline delimiter_index_type const value{t_delimiter == t_first ? t_i : index_of <t_delimiter, t_i + 1, t_rest...>::value};
	};
}


namespace libbio::parsing {

	template <delimiter_type... t_delimiters>
	struct delimiter
	{
		typedef detail::first_delimiter_type_t <t_delimiters...>	type;
		
		template <delimiter_type t_delimiter>
		constexpr static inline auto const index_of_v{detail::index_of <t_delimiter, 0, t_delimiters...>::value};
		
		constexpr static inline std::size_t size() { return sizeof...(t_delimiters); }
		constexpr static inline std::size_t last_index() { return size() - 1; }
		constexpr static inline bool matches(type const other) { return ((other == t_delimiters) || ...); }
		static inline delimiter_index_type matching_index(type const other);
	};


	template <delimiter_type... t_delimiters>
	delimiter_index_type delimiter <t_delimiters...>::matching_index(type const other)
	{
		// Returns the number of the passed delimiters if not found.
		delimiter_index_type retval{};
		(((other == t_delimiters) || (++retval, false)) || ...);
		return retval;
	}
	
	
	template <typename, typename>
	struct join_delimiters
	{
	};
	
	template <delimiter_type... t_delimiters_1, delimiter_type... t_delimiters_2>
	struct join_delimiters <delimiter <t_delimiters_1...>, delimiter <t_delimiters_2...>>
	{
		typedef delimiter <t_delimiters_1..., t_delimiters_2...> type;
	};
	
	template <typename t_lhs, typename t_rhs>
	using join_delimiters_t = typename join_delimiters <t_lhs, t_rhs>::type;
}

#endif

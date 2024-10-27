/*
 * Copyright (c) 2023-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_SAM_LITERALS_HH
#define LIBBIO_SAM_LITERALS_HH

#include <array>
#include <cstddef>
#include <libbio/assert.hh>
#include <libbio/sam/cigar.hh>
#include <libbio/sam/tag.hh>
#include <type_traits>


namespace libbio::sam::detail {

	template <typename t_type>
	struct is_std_array : public std::false_type {};

	template <typename ... t_args>
	struct is_std_array <std::array <t_args...>> : public std::true_type {};

	template <typename t_type>
	constexpr static inline auto const is_std_array_v{is_std_array <t_type>::value};


	template <std::size_t t_n>
	struct string_literal
	{
		std::array <char, t_n>	value{};

		template <std::size_t t_i>
		constexpr /* implicit */ string_literal(char const (&value_)[t_i])
		{
			std::copy_n(&value_[0], t_n, value.begin());
		}

		template <typename t_array>
		requires is_std_array_v <t_array>
		constexpr /* implicit */ string_literal(t_array &&value_):
			value(std::forward <t_array>(value))
		{
		}
	};

	template <std::size_t t_i> string_literal(char const (&value)[t_i]) -> string_literal <t_i - 1>;
}



namespace libbio::sam { inline namespace literals {

	inline namespace cigar_operation_literals {

		constexpr inline cigar_operation operator ""_cigar_operation(char const op) { return make_cigar_operation(op); }
	}


	inline namespace tag_type_literals {

		template <detail::string_literal <2> t_tag>
		constexpr inline tag_type operator ""_tag()
		{
			return to_tag(t_tag.value);
		}
	}
}}

#endif

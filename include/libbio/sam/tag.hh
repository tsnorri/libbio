/*
 * Copyright (c) 2023 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_SAM_TAG_HH
#define LIBBIO_SAM_TAG_HH

#include <array>
#include <cstdint>
#include <libbio/assert.hh>
#include <vector>


namespace libbio::sam {
	
	typedef std::uint16_t			tag_type;
	typedef std::vector <tag_type>	tag_vector;
	
	
	// Convert a SAM tag to a std::array <char, N> where 2 ≤ N.
	template <std::size_t t_size>
	constexpr inline void from_tag(tag_type const val, std::array <char, t_size> &buffer)
	requires (2 <= t_size)
	{
		char const char0(val >> 8); // Narrowed automatically when () (not {}) are used.
		char const char1(val & 0xff);
		std::get <0>(buffer) = char0;
		std::get <1>(buffer) = char1;
	}
	
	
	// Convert a std::array <char, 2> to a SeqAn 3 SAM tag.
	constexpr inline tag_type to_tag(std::array <char, 2> const &buffer)
	{
		// The tag needs to match /[A-Za-z][A-Za-z0-9]/ (SAMv1, Section 1.5 The alignment section: optional fields),
		// so the values will not be negative.
		libbio_always_assert_lte(std::get <0>(buffer), 127);
		libbio_always_assert_lte(std::get <1>(buffer), 127);
		tag_type retval(std::get <0>(buffer)); // Narrows when () (not {}) are used.
		retval <<= 8;
		retval |= std::get <1>(buffer);
		return retval;
	}
	
	
	constexpr inline tag_type operator ""_tag(char const *str, std::size_t const size)
	{
		libbio_assert_eq(2, size);
		return to_tag(std::array{str[0], str[1]});
	}
	
	
	template <tag_type t_tag> struct tag_value {};
	
	template <> struct tag_value <"NM"_tag> { typedef std::int32_t	type; };
	template <> struct tag_value <"OA"_tag> { typedef std::string	type; };
	
	template <tag_type t_tag>
	using tag_value_t = tag_value <t_tag>::type;
}

#endif

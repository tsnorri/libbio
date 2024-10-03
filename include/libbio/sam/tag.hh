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


namespace libbio::sam::detail {
	
	constexpr inline bool check_tag_character_1(char const cc)
	{
		return ('a' <= cc && cc <= 'z') || ('A' <= cc && cc <= 'Z');
	}
	
	constexpr inline bool check_tag_character_2(char const cc)
	{
		return ('0' <= cc && cc <= '9') || ('a' <= cc && cc <= 'z') || ('A' <= cc && cc <= 'Z');
	}
}


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
	
	
	// Convert a std::span <char, N> to a SAM tag.
	template <std::size_t t_n>
	requires (2 <= t_n) // allows std::dynamic_extent
	constexpr inline tag_type to_tag_(std::span <char const, t_n> const &span)
	{
		// The tag needs to match /[A-Za-z][A-Za-z0-9]/ (SAMv1, Section 1.5 The alignment section: optional fields),
		// so the values will not be negative.
		libbio_always_assert(detail::check_tag_character_1(span[0]));
		libbio_always_assert(detail::check_tag_character_2(span[1]));
		tag_type retval(span[0]); // Narrows when () (not {}) are used.
		retval <<= 8;
		retval |= span[1];
		return retval;
	}
	
	template <std::size_t t_n>
	requires (2 == t_n)
	constexpr inline tag_type to_tag(std::span <char const, t_n> const &span) { return to_tag_(span); }
	
	
	constexpr inline tag_type to_tag(std::array <char, 2> const &arr) { return to_tag_(std::span{arr}); }
	
	
	template <tag_type t_tag> struct tag_value {};
	
	template <tag_type t_tag>
	using tag_value_t = tag_value <t_tag>::type;
}

#endif

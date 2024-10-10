/*
 * Copyright (c) 2018-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_UTILITY_MISC_HH
#define LIBBIO_UTILITY_MISC_HH

#include <array>
#include <boost/core/demangle.hpp>
#include <boost/format.hpp>
#include <charconv>
#include <cstring>
#include <ostream>
#include <string>
#include <type_traits>	// std::integral_constant
#include <utility>


namespace libbio {
	
	template <size_t t_size>
	using size_constant = std::integral_constant <std::size_t, t_size>;
	
	
	template <typename t_type, std::size_t t_size>
	consteval std::size_t array_size(t_type const (&array)[t_size]) { return t_size; }
	
	
	template <typename... t_args>
	struct aggregate : public t_args...
	{
		using t_args::operator()...;
		constexpr aggregate(t_args && ... args): t_args(std::move(args))... {}
	};
	
	
	template <typename... t_args>
	constexpr bool any(t_args && ... args)
	{
		return (... || args);
	}
	
	
	template <typename... t_args>
	constexpr bool all(t_args && ... args)
	{
		return (... && args);
	}
	
	
	template <typename t_type, bool t_condition>
	using add_const_if_t = std::conditional_t <t_condition, std::add_const_t <t_type>, t_type>;
	
	
	template <std::size_t t_address, std::size_t t_alignment>
	struct next_aligned_address : public std::integral_constant <std::size_t,
		(0 == t_address % t_alignment)
		? t_address
		: (t_address + (t_alignment - t_address % t_alignment))
	> {};
	
	template <std::size_t t_address, std::size_t t_alignment>
	constexpr static inline auto const next_aligned_address_v{next_aligned_address <t_address, t_alignment>::value};
	
	
	inline char *copy_format_cstr(boost::format const &fmt) { return strdup(boost::str(fmt).c_str()); }
	
	std::ostream &log_time(std::ostream &stream);
	std::string copy_time();

	template <typename t_stream>
	t_stream &&log_time(t_stream &&stream)
	requires std::is_rvalue_reference_v <decltype(stream)>
	// FIXME: add requires for checking base class?
	{
		log_time(stream);
		return std::move(stream);
	}
	
	// Calculate the printed length of a UTF-8 string by checking the first two bits of each byte.
	std::size_t strlen_utf8(std::string const &str);
	
	
	template <typename t_type>
	std::string type_name() { return boost::core::demangle(typeid(t_type).name()); }
	
	
	template <typename t_enum>
	constexpr std::underlying_type_t <t_enum> to_underlying(t_enum const en)
	{
		return static_cast <std::underlying_type_t <t_enum>>(en);
	}
	
	
	// Comparison helper.
	template <typename t_lhs, typename t_rhs>
	struct compare_lhs_first_lt
	{
		bool operator()(t_lhs const &lhs, t_rhs const &rhs) const
		{
			return lhs.first < rhs;
		}
	};
	
	
	template <typename t_src, typename t_dst>
	void resize_and_copy(t_src const &src, t_dst &dst)
	{
		dst.resize(src.size());
		std::copy(src.begin(), src.end(), dst.begin());
	}
	
	
	template <typename t_vector>
	void clear_and_resize_vector(t_vector &vec)
	{
		// Swap with an empty vector.
		t_vector empty_vec;
		
		using std::swap;
		swap(vec, empty_vec);
	}
	
	
	// Simpler variant of std::experimental::make_array.
	template <typename t_value, typename ... t_args>
	constexpr auto make_array(t_args && ... args) -> std::array <t_value, sizeof...(t_args)>
	{
		return {std::forward <t_args>(args)...};
	}
	
	
	template <typename t_coll>
	void resize_and_fill_each(t_coll &vec_collection, std::size_t const size)
	{
		for (auto &vec : vec_collection)
		{
			vec.resize(size);
			std::fill(vec.begin(), vec.end(), 0);
		}
	}
	
	
	template <typename t_vec>
	void resize_and_zero(t_vec &vec, std::size_t const size)
	{
		vec.resize(size);
		std::fill(vec.begin(), vec.end(), 0);
	}
	
	
	template <typename t_dst>
	bool parse_integer(std::string_view const &str, t_dst &dst)
	{
		auto const res(std::from_chars(str.data(), str.data() + str.size(), dst));
		return (std::errc() == res.ec);
	}
}

#endif

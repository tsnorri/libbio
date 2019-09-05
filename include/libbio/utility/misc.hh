/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_UTILITY_MISC_HH
#define LIBBIO_UTILITY_MISC_HH

#include <array>
#include <boost/core/demangle.hpp>
#include <boost/format.hpp>
#include <cstring>
#include <ostream>
#include <string>
#include <utility>


namespace libbio {

	inline char *copy_format_cstr(boost::format const &fmt) { return strdup(boost::str(fmt).c_str()); }
	
	void log_time(std::ostream &stream);
	std::string copy_time();
	
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
}

#endif

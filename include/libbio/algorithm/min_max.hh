/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_ALGORITHM_MIN_MAX_HH
#define LIBBIO_ALGORITHM_MIN_MAX_HH

#include <algorithm>
#include <climits>
#include <libbio/assert.hh>


namespace libbio {
	
	template <typename t_it>
	constexpr std::size_t argmax_element(t_it begin, t_it end)
	{
		return std::distance(begin, std::max_element(begin, end));
	}
	
	
	template <typename t_it>
	constexpr std::size_t argmin_element(t_it begin, t_it end)
	{
		return std::distance(begin, std::min_element(begin, end));
	}
	
	
	// Return the indices of the maximum elements as a bit mask.
	template <typename t_it>
	constexpr std::uint64_t argmax_elements(t_it it, t_it end)
	{
		typedef typename std::iterator_traits <t_it>::value_type value_type;
		auto max_val(std::numeric_limits <value_type>::min());

		std::uint64_t mask{0x1};
		std::uint64_t retval{};
		
		libbio_assert(std::distance(it, end) <= 64);
		while (it != end)
		{
			auto val(*it);
			if (max_val == val)
				retval |= mask;
			else if (max_val < val)
			{
				retval = mask;
				max_val = val;
			}
			
			mask <<= 1;
		}
		
		return retval;
	}
	
	
	template <typename t_type_1, typename t_type_2>
	constexpr auto max_ct(t_type_1 const &val_1, t_type_2 const &val_2) -> std::common_type_t <t_type_1, t_type_2>
	{
		typedef std::common_type_t <t_type_1, t_type_2> return_type;
		return_type retval(std::max <return_type>(val_1, val_2));
		libbio_assert(val_1 <= retval);
		libbio_assert(val_2 <= retval);
		return retval;
	}
	
	
	template <typename t_type_1, typename t_type_2>
	constexpr auto min_ct(t_type_1 const &val_1, t_type_2 const &val_2) -> std::common_type_t <t_type_1, t_type_2>
	{
		typedef std::common_type_t <t_type_1, t_type_2> return_type;
		return_type retval(std::min <return_type>(val_1, val_2));
		libbio_assert(retval <= val_1);
		libbio_assert(retval <= val_2);
		return retval;
	}
}

#endif

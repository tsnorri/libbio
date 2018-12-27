/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_ALGORITHM_SET_COUNT_HH
#define LIBBIO_ALGORITHM_SET_COUNT_HH

#include <cstddef>
#include <iterator>


namespace libbio {
	
	// FIXME: constexpr in C++20?
	template <typename t_input_it_1, typename t_input_it_2>
	std::size_t set_symmetric_difference_size(
		t_input_it_1 first_1,
		t_input_it_1 last_1,
		t_input_it_2 first_2,
		t_input_it_2 last_2
	)
	{
		std::size_t retval(0);
		while (first_1 != last_1)
		{
			if (first_2 == last_2)
			{
				retval += std::distance(first_1, last_1);
				return retval;
			}
			
			if (*first_1 < *first_2)
			{
				++retval;
				++first_1;
			}
			else if (*first_2 < *first_1)
			{
				++retval;
				++first_2;
			}
			else
			{
				// *first_1 == *first_2.
				++first_1;
				++first_2;
			}
		}
		
		retval += std::distance(first_2, last_2);
		return retval;
	}
	
	
	// FIXME: constexpr in C++20?
	template <typename t_input_it_1, typename t_input_it_2>
	std::size_t set_intersection_size(
		t_input_it_1 first_1,
		t_input_it_1 last_1,
		t_input_it_2 first_2,
		t_input_it_2 last_2
	)
	{
		std::size_t retval(0);
		while (first_1 != last_1)
		{
			if (first_2 == last_2)
				return retval;
			
			if (*first_1 < *first_2)
				++first_1;
			else if (*first_2 < *first_1)
				++first_2;
			else
			{
				// *first_1 == *first_2.
				++retval;
				++first_1;
				++first_2;
			}
		}
		
		return retval;
	}
}

#endif

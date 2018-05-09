/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_ALGORITHM_HH
#define LIBBIO_ALGORITHM_HH


namespace libbio {
	
	template <typename t_type_1, typename t_type_2>
	constexpr std::common_type_t <t_type_1, t_type_2> const max_ct(t_type_1 const &val_1, t_type_2 const &val_2)
	{
		typedef std::common_type_t <t_type_1, t_type_2> return_type;
		return_type retval(std::max <return_type>(val_1, val_2));
		assert(val_1 <= retval);
		assert(val_2 <= retval);
		return retval;
	}
	
	
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
}

#endif

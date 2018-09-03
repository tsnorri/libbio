/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_RADIX_SORT_HH
#define LIBBIO_RADIX_SORT_HH

#include <libbio/bits.hh>


namespace libbio {
	
	// Stable LSB radix sort, requires n + O(1) extra space.
	template <typename t_container>
	void radix_sort(
		t_container &container,
		t_container &buffer,
		std::size_t const bit_limit = CHAR_BIT * sizeof(typename t_container::value_type)
	)
	{
		auto const size(container.size());
		
		if (0 == size)
			return;
		
		buffer.resize(container.size());
		
		using std::swap;
		
		std::size_t shift_amt(0);
		while (shift_amt < bit_limit)
		{
			std::size_t fidx(0);
			std::size_t ridx_1(size);
			
			for (auto const val : container)
			{
				if ((val >> shift_amt) & 0x1)
					buffer[--ridx_1] = val;
				else
					buffer[fidx++] = val;
			}
			
			assert(fidx = ridx_1);
			std::reverse(buffer.begin() + ridx_1, buffer.end());
			swap(container, buffer);
			++shift_amt;
		}
	}
	
	
	// Iterate one time to determine the highest bit set.
	template <typename t_container>
	void radix_sort_check_bits_set(
		t_container &container,
		t_container &buffer
	)
	{
		std::uint32_t lz_count(sizeof(typename t_container::value_type) * CHAR_BIT);
		for (auto const val : container)
			lz_count = std::min(lz_count, leading_zeros(val));
		
		radix_sort(container, buffer, (sizeof(typename t_container::value_type) * CHAR_BIT) - lz_count);
	}
}

#endif

/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_RADIX_SORT_HH
#define LIBBIO_RADIX_SORT_HH

#include <libbio/algorithm.hh>
#include <libbio/bits.hh>
#include <type_traits>


namespace libbio { namespace detail {
	
	template <typename t_value>
	struct identity_access
	{
		t_value operator()(t_value value) { return value; }
	};
	
	template <typename t_fn, typename t_arg>
	constexpr std::size_t return_type_size() { return sizeof(std::invoke_result_t <t_fn, t_arg>); }

	
	template <bool t_reverse>
	struct radix_sort_reverse
	{
		static constexpr bool should_push_back(bool const is_one) { return is_one; }
	};
	
	template <>
	struct radix_sort_reverse <true>
	{
		static constexpr bool should_push_back(bool const is_one) { return !is_one; }
	};
}}


namespace libbio {
	
	template <bool t_reverse = false>
	struct radix_sort
	{
		// Stable LSB radix sort, requires n + O(1) extra space.
		template <typename t_container, typename t_access>
		static void sort(
			t_container &container,
			t_container &buffer,
			t_access &&access,
			std::size_t const bit_limit = CHAR_BIT * detail::return_type_size <t_access, typename t_container::value_type>()
		)
		{
			auto const size(container.size());
			
			if (0 == size)
				return;
			
			buffer.clear();
			buffer.resize(container.size());
			
			using std::swap;
			
			std::size_t shift_amt(0);
			while (shift_amt < bit_limit)
			{
				std::size_t fidx(0);
				std::size_t ridx_1(size);
				
				for (auto it(container.cbegin()), end(container.cend()); it != end; ++it)
				{
					auto const val(access(*it));
					if (detail::radix_sort_reverse <t_reverse>::should_push_back((val >> shift_amt) & 0x1))
						buffer[--ridx_1] = std::move(*it);
					else
						buffer[fidx++] = std::move(*it);
				}
				
				assert(fidx == ridx_1);
				std::reverse(buffer.begin() + ridx_1, buffer.end());
				swap(container, buffer);
				++shift_amt;
			}
		}
		
		
		template <typename t_container>
		static void sort(
			t_container &container,
			t_container &buffer,
			std::size_t const bit_limit = CHAR_BIT * sizeof(typename t_container::value_type)
		)
		{
			detail::identity_access <typename t_container::value_type> access;
			sort(container, buffer, access, bit_limit);
		}
		
		
		// Iterate one time to determine the highest bit set.
		template <typename t_container, typename t_access>
		static void sort_check_bits_set(
			t_container &container,
			t_container &buffer,
			t_access &&access
		)
		{
			std::uint8_t lz_count(sizeof(typename t_container::value_type) * CHAR_BIT);
			for (auto it(container.cbegin()), end(container.cend()); it != end; ++it)
				lz_count = std::min(lz_count, bits::leading_zeros(access(*it)));
			
			sort(container, buffer, std::forward <t_access>(access), (sizeof(typename t_container::value_type) * CHAR_BIT) - lz_count);
		}
		
		
		template <typename t_container>
		static void sort_check_bits_set(
			t_container &container,
			t_container &buffer
		)
		{
			detail::identity_access <typename t_container::value_type> access;
			sort(container, buffer, access);
		}
	};
}

#endif

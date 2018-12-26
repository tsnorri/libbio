/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_ALGORITHM_HH
#define LIBBIO_ALGORITHM_HH

#include <algorithm>
#include <type_traits>


namespace libbio { namespace detail {
	
	struct default_increment
	{
		template <typename t_item>
		void operator()(t_item &item)
		{
			++item.count;
		}
	};
}}


namespace libbio {
	
	template <typename t_type_1, typename t_type_2>
	constexpr std::common_type_t <t_type_1, t_type_2> const max_ct(t_type_1 const &val_1, t_type_2 const &val_2)
	{
		typedef std::common_type_t <t_type_1, t_type_2> return_type;
		return_type retval(std::max <return_type>(val_1, val_2));
		libbio_assert(val_1 <= retval);
		libbio_assert(val_2 <= retval);
		return retval;
	}
	
	
	template <typename t_type_1, typename t_type_2>
	constexpr std::common_type_t <t_type_1, t_type_2> const min_ct(t_type_1 const &val_1, t_type_2 const &val_2)
	{
		typedef std::common_type_t <t_type_1, t_type_2> return_type;
		return_type retval(std::min <return_type>(val_1, val_2));
		libbio_assert(retval <= val_1);
		libbio_assert(retval <= val_2);
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
	
	
	template <typename t_iterator, typename t_dst, typename t_increment_count>
	void unique_count(t_iterator &&begin, t_iterator const &&end, t_dst &dst, t_increment_count &&increment)
	{
		dst.clear();
		while (true)
		{
			auto it(std::adjacent_find(begin, end));
			if (it == end)
				break;
			
			// Copy the previous range to the buffer.
			std::copy(begin, it, std::back_inserter(dst));
			
			// Determine the size of the run. We already determined that there is one copy.
			auto val(*it); // Copy.
			increment(val);
			libbio_assert(it + 1 != end);
			auto next(it + 2);
			while (next != end && val == *next)
			{
				increment(val);
				++next;
			}
			
			// Copy the first element of the range.
			dst.emplace_back(std::move(val));
			
			// Either next == end or *next is the first non-equivalent pair.
			begin = next;
		}
		
		// Copy the last range.
		std::copy(begin, end, std::back_inserter(dst));
	}
	
	
	template <typename t_iterator, typename t_dst>
	void unique_count(t_iterator &&begin, t_iterator const &&end, t_dst &dst)
	{
		detail::default_increment inc;
		unique_count(std::forward <t_iterator>(begin), std::forward <t_iterator const>(end), dst, inc);
	}
}

#endif

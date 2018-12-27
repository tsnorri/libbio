/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_ALGORITHM_UNIQUE_COUNT_HH
#define LIBBIO_ALGORITHM_UNIQUE_COUNT_HH

#include <algorithm>
#include <iterator>
#include <libbio/assert.hh>
#include <utility>


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

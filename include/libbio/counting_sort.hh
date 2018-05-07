/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_COUNTING_SORT_HH
#define LIBBIO_COUNTING_SORT_HH

#include <libbio/array_list.hh>
#include <libbio/assert.hh>


namespace libbio { namespace detail {
	
	// Determine the minimum and maximum value of an identifier accessed with the given functor.
	template <typename t_vector, typename t_identifier, typename t_access>
	void identifier_min_max(t_vector const &src, t_access &access, t_identifier &min_identifier, t_identifier &max_identifier)
	{
		min_identifier = access(src.front());
		max_identifier = access(src.front());
		for (auto const &val : src)
		{
			auto const identifier(access(val));
			if (identifier < min_identifier)
				min_identifier = identifier;
			if (max_identifier < identifier)
				max_identifier = identifier;
		}
	}
	
	
	template <typename t_vector, typename t_count_vector, typename t_access, typename t_identifier>
	void count_items_2(
		t_vector &src,
		t_access &access,
		t_identifier const min_identifier,
		t_identifier const max_identifier,
		t_count_vector &counts
	)
	{
		// Make space for the identifiers.
		auto const identifier_range_size(1 + max_identifier - min_identifier);
		if (counts.size() < identifier_range_size)
			counts.resize(identifier_range_size);
		
		// Link the counts. Assume that max_count does not occur in the list,
		// for which there is an assertion in the end.
		// (Only needed for non-consecutive values.)
		{
			auto const max_count(std::numeric_limits <typename t_count_vector::value>::max());
			std::size_t identifier_count(0);
			for (auto const &val : src)
			{
				auto const identifier(access(val) - min_identifier);
				counts[identifier] = max_count;
				++identifier_count;
			}
		
			counts.set_first_element(0);
			std::size_t prev_idx(0);
			std::size_t found_max_counts(0);
			for (std::size_t i(1); i < identifier_range_size; ++i)
			{
				auto &item(counts.item(i));
				if (max_count == item.value)
				{
					++found_max_counts;
					auto &prev_item(counts.item(prev_idx));
					prev_item.next = i;
					item.prev = prev_idx;
					prev_idx = i;
				}
			}
			counts.set_last_element(prev_idx);
			always_assert(found_max_counts == identifier_count);
		}
		
		// Count the number of each object.
		for (auto const &val : src)
		{
			auto const identifier(access(val) - min_identifier);
			++counts[identifier];
		}
	}
}}


namespace libbio {
	
	// Count items with a custom accessor and store the counts to an array_list.
	template <typename t_vector, typename t_count_vector, typename t_access>
	void count_items(
		t_vector &src,
		t_count_vector &counts,
		t_access &access
	)
	{
		typedef decltype(std::declval <t_access>().operator()(std::declval <typename t_vector::value_type>())) identifier_type;
		typedef typename t_count_vector::value count_value;
		
		if (0 == src.size())
			return;
		
		counts.reset();
		
		// Determine the minimum and maximum identifier values.
		identifier_type min_identifier, max_identifier;
		detail::identifier_min_max(src, access, min_identifier, max_identifier);
		
		// Count the items.
		detail::count_items_2(src, access, min_identifier, max_identifier, counts);
	}
	
	
	// An implementation of counting sort. Requires O(n + σ) time where σ is the
	// alphabet size. Quite a lot of time is spent on setup, though, so may be
	// slow in practice.
	// t_count_vector needs to be an instance of array_list.
	template <typename t_vector, typename t_count_type, typename t_access>
	void counting_sort_al(
		t_vector &src,
		t_vector &dst,
		array_list <t_count_type> &counts,
		t_access &access
	)
	{
		typedef decltype(std::declval <t_access>().operator()(std::declval <typename t_vector::value_type>())) identifier_type;
		
		if (0 == src.size())
			return;
		
		counts.reset();
		
		// Determine the minimum and maximum identifier values.
		identifier_type min_identifier, max_identifier;
		detail::identifier_min_max(src, access, min_identifier, max_identifier);
		
		// Count the items.
		detail::count_items_2(src, access, min_identifier, max_identifier, counts);
		
		// Shift the counts and calculate the cumulative sum.
		{
			using std::swap;
			t_count_type prev_count(0);
			for (auto &item : counts.item_iterator_proxy())
			{
				item.value += prev_count;
				swap(item.value, prev_count);
			}
		}
		
		// Make space for the values.
		if (dst.size() < src.size())
			dst.resize(src.size());
		
		// Move the values to dst.
		for (auto &val : src)
		{
			auto const identifier(access(val) - min_identifier);
			auto const dst_idx(counts[identifier]++);
			dst[dst_idx] = std::move(val);
		}
	}
	
	
	// A simpler version of the algorithm that does not store the counts to an array_list.
	template <typename t_vector, typename t_count_vector, typename t_access>
	void counting_sort(
		t_vector &src,
		t_vector &dst,
		t_count_vector &counts,
		t_access &access
	)
	{
		typedef decltype(std::declval <t_access>().operator()(std::declval <typename t_vector::value_type>())) identifier_type;
		typedef typename t_count_vector::value_type count_value;
		
		if (0 == src.size())
			return;
		
		// Determine the minimum and maximum identifier values.
		identifier_type min_identifier, max_identifier;
		detail::identifier_min_max(src, access, min_identifier, max_identifier);
		
		// Make space for the identifiers.
		auto const identifier_range_size(1 + max_identifier - min_identifier);
		if (counts.size() < identifier_range_size)
			counts.resize(identifier_range_size);
		
		// Set the initial counts to zero.
		std::fill(counts.begin(), counts.begin() + identifier_range_size, 0);
		
		// Count the items.
		for (auto const &val : src)
		{
			auto const identifier(access(val) - min_identifier);
			++counts[identifier];
		}
		
		// Shift the counts and calculate the cumulative sum.
		{
			using std::swap;
			count_value prev_count(0);
			for (auto &count : counts)
			{
				count += prev_count;
				swap(count, prev_count);
			}
		}
		
		// Make space for the values.
		if (dst.size() < src.size())
			dst.resize(src.size());
		
		// Move the values to dst.
		for (auto &val : src)
		{
			auto const identifier(access(val) - min_identifier);
			auto const dst_idx(counts[identifier]++);
			dst[dst_idx] = std::move(val);
		}
	}
}

#endif

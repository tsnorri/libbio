/*
 * Copyright (c) 2023 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_ALGORITHM_STABLE_PARTITION_LEFT_HH
#define LIBBIO_ALGORITHM_STABLE_PARTITION_LEFT_HH

#include <algorithm>
#include <functional>		// std::identity
#include <libbio/assert.hh>

namespace libbio::detail {
	
	// Remove the values at the given indices.
	template <typename t_values_it, typename t_idxs_it, typename t_proj, typename t_shift>
	[[nodiscard]] t_values_it shift_at_indices(
		t_values_it values_it,
		t_values_it const values_end,
		t_idxs_it idxs_it, 
		t_idxs_it const idxs_end,
		t_proj &&proj,
		t_shift &&shift
	)
	{
		if (idxs_it == idxs_end)
			return values_end;
		
#if !(defined(LIBBIO_NDEBUG) && LIBBIO_NDEBUG)
		auto const size(std::distance(values_it, values_end));
#endif
		
		auto prev_idx{proj(*idxs_it)};
		auto range_start{prev_idx};
		static_assert(std::is_unsigned_v <decltype(prev_idx)>); // Sanity check.
		libbio_assert_lt(prev_idx, size);
		
		++idxs_it;
		while (idxs_it != idxs_end)
		{
			auto const idx(proj(*idxs_it));
			libbio_assert_lt(idx, size);
			if (1 + prev_idx != idx)
			{
				shift(values_it + range_start, values_it + prev_idx + 1, values_it + idx);
				range_start += idx - (prev_idx + 1);
			}
			
			prev_idx = idx;
			++idxs_it;
		}
		
		return shift(values_it + range_start, values_it + prev_idx + 1, values_end);
	}
}


namespace libbio {
	
	// Stable partition implemented in such a way that the relative order on the left hand side is preserved.
	// (This is not guaranteed in std::stable_partition.)
	template <typename t_iterator, typename t_predicate>
	t_iterator stable_partition_left(t_iterator left, t_iterator const right, t_predicate &&pred)
	{
		left = std::find_if_not(left, right, pred);
		if (right == left)
			return right;
		
		for (auto mid(left); ++mid != right;)
		{
			if (pred(*mid))
			{
				std::iter_swap(left, mid);
				++left;
			}
		}
		
		return left;
	}
	
	
	// Similar to stable_partition_left but insteaad of a predicate a range of indices is used.
	// FIXME: combine with remove_at_indices since the only difference is the use of std::rotate instead of std::move.
	template <typename t_values_it, typename t_idxs_it, typename t_proj>
	[[nodiscard]] t_values_it stable_partition_left_at_indices(
		t_values_it values_it,
		t_values_it const values_end,
		t_idxs_it idxs_it, 
		t_idxs_it const idxs_end,
		t_proj &&proj
	)
	{
		struct shift
		{
			template <typename t_iterator>
			t_iterator operator(t_iterator first, t_iterator mid, t_iterator last)
			{
				return std::rotate(first, mid, last);
			}
		};
		
		return detail::shift_at_indices(values_it, values_end, idxs_it, idxs_end, proj, shift{});
	}
	
	
	template <typename t_values_it, typename t_idxs_it>
	[[nodiscard]] t_values_it stable_partition_left_at_indices(
		t_values_it values_it,
		t_values_it const values_end,
		t_idxs_it idxs_it, 
		t_idxs_it const idxs_end
	)
	{
		return stable_partition_left_at_indices(values_it, values_end, idxs_it, idxs_end, std::identity{});
	}
	
	
	template <typename t_values_it, typename t_idxs_it, typename t_proj>
	[[nodiscard]] t_values_it remove_at_indices(
		t_values_it values_it,
		t_values_it const values_end,
		t_idxs_it idxs_it, 
		t_idxs_it const idxs_end,
		t_proj &&proj
	)
	{
		struct shift
		{
			template <typename t_iterator>
			t_iterator operator(t_iterator first, t_iterator mid, t_iterator last)
			{
				return std::move(mid, last, first);
			}
		};
		
		return detail::shift_at_indices(values_it, values_end, idxs_it, idxs_end, proj, shift{});
	}
	
	
	template <typename t_values_it, typename t_idxs_it>
	[[nodiscard]] t_values_it remove_at_indices(
		t_values_it values_it,
		t_values_it const values_end,
		t_idxs_it idxs_it, 
		t_idxs_it const idxs_end
	)
	{
		return remove_at_indices(values_it, values_end, idxs_it, idxs_end, std::identity{});
	}
}

#endif

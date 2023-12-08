/*
 * Copyright (c) 2023 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_ALGORITHM_SET_DIFFERENCE_INPLACE_HH
#define LIBBIO_ALGORITHM_SET_DIFFERENCE_INPLACE_HH

#include <algorithm>
#include <functional>		// std::identity
#include <libbio/assert.hh>


namespace libbio {
	
	// Remove the values at the given indices.
	template <typename t_values_it, typename t_idxs_it, typename t_proj>
	[[nodiscard]] t_values_it remove_at_indices(
		t_values_it values_it,
		t_values_it const values_end,
		t_idxs_it idxs_it, 
		t_idxs_it const idxs_end,
		t_proj &&proj
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
				std::move(values_it + prev_idx + 1, values_it + idx, values_it + range_start);
				range_start += idx - (prev_idx + 1);
			}
			
			prev_idx = idx;
			++idxs_it;
		}
		
		return std::move(values_it + prev_idx + 1, values_end, values_it + range_start);
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
	
	
	// Computes the difference of the two given sorted sets in such a way that the first is overwritten by the result.
	template <typename t_dst_it, typename t_matching_it, typename t_cmp>
	[[nodiscard]] t_dst_it set_difference_inplace(
		t_dst_it dst_it,
		t_dst_it const dst_end,
		t_matching_it matching_it, 
		t_matching_it const matching_end,
		t_cmp &&cmp
	)
	{
		// Assume that the sets are sorted using the same order relation.
		// Find the first matching element in dst with binary search.
		while (true)
		{
			if (matching_it == matching_end)
				return dst_end;
			
			auto const &mm(*matching_it);
			dst_it = std::lower_bound(dst_it, dst_end, mm, cmp);
			
			if (dst_end == dst_it)
				return dst_end;
			
			auto const &rr(*dst_it);
			if (cmp(mm, rr))
			{
				// mm < rr, retry with the next element in matched.
				++matching_it;
				continue;
			}
			
			// mm = rr.
			++matching_it;
			break;
		}
		
		// Continue by shifting.
		auto it(dst_it);
		++it;
		while (it != dst_end)
		{
			auto &rr(*it);
			auto const &mm(*matching_it);
			
			if (cmp(mm, rr))
			{
				// mm < rr.
				++matching_it;
				if (matching_end == matching_it)
					break;
				
				continue;
			}
			
			if (cmp(rr, mm))
			{
				// rr < mm; preserve rr, advance dst.
				*dst_it = std::move(rr);
				++it;
				++dst_it;
				continue;
			}
			
			// rr = mm.
			++it;
			++matching_it;
			if (matching_end == matching_it)
				break;
		}
		
		return std::move(it, dst_end, dst_it);
	}
}

#endif

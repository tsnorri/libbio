/*
 * Copyright (c) 2023 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_ALGORITHM_SORTED_SET_UNION_HH
#define LIBBIO_ALGORITHM_SORTED_SET_UNION_HH

#include <libbio/assert.hh>
#include <range/v3/all.hpp>
#include <type_traits>

namespace libbio::detail {
	
	struct sorted_set_default_cmp
	{
		template <typename t_lhs, typename t_rhs>
		bool operator()(t_lhs const &lhs, t_rhs const &rhs) const
		{
			// Should we use the spaceship operator instead?
			return lhs < rhs;
		}
	};
}


namespace libbio {
	
	// Merge two sorted views to an output iterator. The elements in both views must be distinct w.r.t. cmp.
	template <typename t_lhs_it, typename t_lhs_sentinel, typename t_rhs_it, typename t_rhs_sentinel, typename t_output_it, typename t_cmp>
	t_output_it sorted_set_union(t_lhs_it lhs_it, t_lhs_sentinel lhs_end, t_rhs_it rhs_it, t_rhs_sentinel rhs_end, t_output_it output_it, t_cmp &&cmp)
	{
		while (lhs_it != lhs_end)
		{
			if (rhs_it == rhs_end)
				return std::copy(lhs_it, lhs_end, output_it);
			
			if (cmp(*lhs_it, *rhs_it))
			{
				*output_it = *lhs_it;
				++output_it;
				++lhs_it;
				continue;
			}
			
			if (cmp(*rhs_it, *lhs_it))
			{
				*output_it = *rhs_it;
				++output_it;
				++rhs_it;
				continue;
			}
			
			// *lhs_it == *rhs_it.
			*output_it = *lhs_it;
			++output_it;
			++lhs_it;
			++rhs_it;
		}
		
		return std::copy(rhs_it, rhs_end, output_it);
	}
	
	
	template <typename t_lhs_it, typename t_lhs_sentinel, typename t_rhs_it, typename t_rhs_sentinel, typename t_output_it>
	t_output_it sorted_set_union(t_lhs_it lhs_it, t_lhs_sentinel lhs_end, t_rhs_it rhs_it, t_rhs_sentinel rhs_end, t_output_it output_it)
	{
		detail::sorted_set_default_cmp cmp;
		return sorted_set_union(lhs_it, lhs_end, rhs_it, rhs_end, output_it, cmp);
	}
	
	
	template <typename t_lhs_range, typename t_rhs_range, typename t_output_it, typename t_cmp>
	t_output_it sorted_set_union(t_lhs_range lhs_range, t_rhs_range rhs_range, t_output_it output_it, t_cmp &&cmp)
	{
		auto l_begin(ranges::begin(lhs_range));
		auto r_begin(ranges::begin(rhs_range));
		auto l_end(ranges::end(lhs_range));
		auto r_end(ranges::end(rhs_range));
		return sorted_set_union(l_begin, l_end, r_begin, r_end, output_it, cmp);
	}
	
	
	template <typename t_lhs_range, typename t_rhs_range, typename t_output_it>
	t_output_it sorted_set_union(t_lhs_range lhs_range, t_rhs_range rhs_range, t_output_it output_it)
	{
		detail::sorted_set_default_cmp cmp;
		return sorted_set_union(lhs_range, rhs_range, output_it, cmp);
	}
}

#endif

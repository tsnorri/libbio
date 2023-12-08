/*
 * Copyright (c) 2023 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_ALGORITHM_STABLE_PARTITION_LEFT_HH
#define LIBBIO_ALGORITHM_STABLE_PARTITION_LEFT_HH

#include <algorithm>


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
}

#endif

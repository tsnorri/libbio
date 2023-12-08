/*
 * Copyright (c) 2023 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <libbio/algorithm/set_difference_inplace.hh>
#include "rapidcheck_test_driver.hh"

namespace lb		= libbio;

typedef std::int32_t value_type;



TEST_CASE(
	"set_difference_inplace with arbitrary disjoint input",
	"[set_difference_inplace]"
)
{
	return lb::rc_check(
		"set_difference_inplace works with arbitrary disjoint input",
		[](std::set <value_type> const &target_, std::set <value_type> const &matched_){
			
			std::vector target(target_.begin(), target_.end());
			
			// Make sure that matched does not overlap with target.
			std::vector matched;
			std::set_difference(matched_.begin(), matched_.end(), target.begin(), target.end(), std::back_inserter(matched));
			
			RC_TAG(matched.empty());
			
			// Compute the expected elements with std::set_difference.
			std::vector expected;
			std::set_difference(target.begin(), target.end(), matched.begin(), matched.end(), std::back_inserter(expected));
			
			// Apply our algorithm.
			auto const it(lb::set_difference_inplace(target.begin(), target.end(), matched.begin(), matched.end(), std::less{}));
			target.erase(it, target.end());
			
			RC_ASSERT(target == expected);
		}
	);
}


TEST_CASE(
	"set_difference_inplace with arbitrary intersecting input",
	"[set_difference_inplace]"
)
{
	return lb::rc_check(
		"set_difference_inplace works with arbitrary intersecting input",
		[](std::set <value_type> const &target_, std::set <value_type> &matched_, std::uint_fast32_t const seed){
			
			std::vector target(target_.begin(), target_.end());
			std::mt19937 gen(seed);
			
			{
				// Add some elements from target to matched.
				
				auto const added_count(target.size() - *gen::inRange(0, target_.size()));
				RC_TAG(added_count);
				
				std::vector added(target);
				std::shuffle(added.begin(), added.end(), gen);
				added.resize(added_count);
				matched_.insert(added.begin(), added.end());
			}
			
			std::vector matched(matched_.begin(), matched_.end());
			
			// Compute the expected elements with std::set_difference.
			std::vector expected;
			std::set_difference(target.begin(), target.end(), matched.begin(), matched.end(), std::back_inserter(expected));
			
			// Apply our algorithm.
			auto const it(lb::set_difference_inplace(target.begin(), target.end(), matched.begin(), matched.end(), std::less{}));
			target.erase(it, target.end());
			
			RC_ASSERT(target == expected);
		}
	);
}

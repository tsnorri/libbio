/*
 * Copyright (c) 2023-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <libbio/algorithm/set_difference_inplace.hh>
#include <libbio/rapidcheck_test_driver.hh>
#include <random>
#include <range/v3/algorithm/copy.hpp>
#include <range/v3/iterator/stream_iterators.hpp>
#include <set>
#include <vector>

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
			std::vector <value_type> matched;
			std::set_difference(matched_.begin(), matched_.end(), target.begin(), target.end(), std::back_inserter(matched));

			RC_TAG("Matched is empty", matched.empty());

			// Compute the expected elements with std::set_difference.
			std::vector <value_type> expected;
			std::set_difference(target.begin(), target.end(), matched.begin(), matched.end(), std::back_inserter(expected));

			// Apply our algorithm.
			auto const it(lb::set_difference_inplace(target.begin(), target.end(), matched.begin(), matched.end(), std::less{}));
			target.erase(it, target.end());

			RC_LOG() << "target:   ";
			ranges::copy(target, ranges::make_ostream_joiner(RC_LOG(), ","));
			RC_LOG() << '\n';
			RC_LOG() << "expected: ";
			ranges::copy(expected, ranges::make_ostream_joiner(RC_LOG(), ","));
			RC_LOG() << '\n';

			return true;
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
		[](std::set <value_type> const &target_, std::set <value_type> const &matched__, std::uint_fast32_t const seed){
			std::vector target(target_.begin(), target_.end());
			auto matched_(matched__);
			std::mt19937 gen(seed);

			{
				// Add some elements from target to matched.

				auto const added_count(target.size() - *rc::gen::inRange(std::size_t(0), target_.size()));

				std::vector added(target);
				std::shuffle(added.begin(), added.end(), gen);
				added.resize(added_count);
				matched_.insert(added.begin(), added.end());

				auto const added_pct(double(added_count) / matched_.size());
				RC_CLASSIFY(added_pct < 0.25);
				RC_CLASSIFY(0.25 <= added_pct && added_pct < 0.5);
				RC_CLASSIFY(0.5 <= added_pct && added_pct < 0.75);
				RC_CLASSIFY(0.75 <= added_pct && added_pct < 1.0);
				RC_CLASSIFY(1.0 == added_pct);
			}

			std::vector matched(matched_.begin(), matched_.end());

			// Compute the expected elements with std::set_difference.
			std::vector <value_type> expected;
			std::set_difference(target.begin(), target.end(), matched.begin(), matched.end(), std::back_inserter(expected));

			// Apply our algorithm.
			auto const it(lb::set_difference_inplace(target.begin(), target.end(), matched.begin(), matched.end(), std::less{}));
			target.erase(it, target.end());

			RC_LOG() << "target:   ";
			ranges::copy(target, ranges::make_ostream_joiner(RC_LOG(), ","));
			RC_LOG() << '\n';
			RC_LOG() << "expected: ";
			ranges::copy(expected, ranges::make_ostream_joiner(RC_LOG(), ","));
			RC_LOG() << '\n';
			RC_ASSERT(target == expected);

			return true;
		}
	);
}

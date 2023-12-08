/*
 * Copyright (c) 2023 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <algorithm>
#include <libbio/algorithm/stable_partition_left.hh>
#include <range/v3/algorithm/copy.hpp>
#include <range/v3/iterator/stream_iterators.hpp>
#include "rapidcheck_test_driver.hh"

namespace lb		= libbio;

typedef std::int32_t value_type;


TEST_CASE(
	"stable_partition_left with arbitrary input",
	"[stable_partition_left]"
)
{
	return lb::rc_check(
		"stable_partition_left works with arbitrary input",
		[](std::vector <value_type> const &values){
			auto const pivot_index(*rc::gen::inRange(std::size_t(0), values.size()));
			auto const pivot(values[pivot_index]);
			auto const pred([pivot](auto const val){ return val < pivot; });
			
			std::vector expected(values);
			std::vector actual(values);
			
			auto const expected_mid(std::stable_partition(expected.begin(), expected.end(), pred));
			auto const actual_mid(lb::stable_partition_left(actual.begin(), actual.end(), pred));
			std::sort(expected_mid, expected.end());
			std::sort(actual_mid, actual.end());
			
			RC_LOG() << "actual:   ";
			ranges::copy(actual, ranges::make_ostream_joiner(RC_LOG(), ","));
			RC_LOG() << '\n';
			RC_LOG() << "expected: ";
			ranges::copy(expected, ranges::make_ostream_joiner(RC_LOG(), ","));
			RC_LOG() << '\n';
			
			RC_ASSERT(std::distance(actual.begin(), actual_mid) == std::distance(expected.begin(), expected_mid));
			RC_ASSERT(std::equal(actual.begin(), actual.end(), expected.begin(), expected.end()));
			
			RC_CLASSIFY(std::is_sorted(values.begin(), values.end()));
			RC_CLASSIFY(actual.begin() == actual_mid);
			RC_CLASSIFY(actual.end() == actual_mid);
			
			return true;
		}
	);
}

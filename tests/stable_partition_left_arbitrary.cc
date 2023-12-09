/*
 * Copyright (c) 2023 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <algorithm>
#include <libbio/algorithm/stable_partition_left.hh>
#include <range/v3/algorithm/copy.hpp>
#include <range/v3/iterator/stream_iterators.hpp>
#include <range/v3/view/reverse.hpp>
#include "rapidcheck_test_driver.hh"

namespace lb		= libbio;
namespace rsv		= ranges::views;

typedef std::int32_t value_type;


namespace {
	
	template <bool t_should_remove>
	auto make_test()
	{
		return [](std::vector <value_type> const &target_){
			auto const indices(*rc::gen::container <std::set <std::size_t>>(rc::gen::inRange(std::size_t(0), target_.size())));
			
			{
				auto const pct(double(indices.size()) / target_.size());
				RC_CLASSIFY(0.0 == pct);
				RC_CLASSIFY(0.0 < pct && pct < 0.25);
				RC_CLASSIFY(0.25 <= pct && pct < 0.5);
				RC_CLASSIFY(0.5 <= pct && pct < 0.75);
				RC_CLASSIFY(0.75 <= pct && pct < 1.0);
				RC_CLASSIFY(1.0 == pct);
			}
			
			auto expected(target_);
			auto actual(target_);
			
			if constexpr (t_should_remove)
			{
				for (auto const idx : rsv::reverse(indices))
					expected.erase(expected.begin() + idx);
				
				auto const it(lb::remove_at_indices(actual.begin(), actual.end(), indices.begin(), indices.end()));
				actual.erase(it, actual.end());
			}
			else
			{
				std::vector <value_type> rest;
				for (auto const idx : rsv::reverse(indices))
				{
					auto it(expected.begin() + idx);
					rest.push_back(*it);
					expected.erase(it);
				}
				
				{
					auto const size(expected.size());
					expected.insert(expected.end(), rest.begin(), rest.end());
					std::sort(expected.begin() + size, expected.end());
				}
				
				auto const it(lb::stable_partition_left_at_indices(actual.begin(), actual.end(), indices.begin(), indices.end()));
				std::sort(it, actual.end());
			}
			
			RC_LOG() << "actual:   ";
			ranges::copy(actual, ranges::make_ostream_joiner(RC_LOG(), ","));
			RC_LOG() << '\n';
			RC_LOG() << "expected: ";
			ranges::copy(expected, ranges::make_ostream_joiner(RC_LOG(), ","));
			RC_LOG() << '\n';
			RC_ASSERT(actual == expected);
			
			return true;
		};
	}
}


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


TEST_CASE(
	"remove_at_indices with arbitrary input",
	"[remove_at_indices]"
)
{
	return lb::rc_check(
		"remove_at_indices works with arbitrary input",
		make_test <true>()
	);
}


TEST_CASE(
	"stable_partition_left_at_indices with arbitrary input",
	"[stable_partition_left_at_indices]"
)
{
	return lb::rc_check(
		"stable_partition_left_at_indices works with arbitrary input",
		make_test <false>()
	);
}

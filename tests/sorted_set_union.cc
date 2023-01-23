/*
 * Copyright (c) 2023 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <catch2/catch.hpp>
#include <libbio/algorithm/sorted_set_union.hh>
#include <rapidcheck.h>
#include <rapidcheck/catch.h>		// rc::prop

namespace lb	= libbio;


TEMPLATE_TEST_CASE(
	"sorted_set_union with integer input",
	"[template][algorithm][sorted_set_union]",
	(signed char),
	(unsigned char),
	(signed short),
	(unsigned short),
	(signed int),
	(unsigned int),
	(signed long),
	(unsigned long),
	(signed long long),
	(unsigned long long)
)
{
	SECTION("sorted_set_union works with empty inputs")
	{
		std::vector <TestType> const lhs, rhs, expected;
		std::vector <TestType> dst;
		
		lb::sorted_set_union(lhs, rhs, std::back_inserter(dst));
		
		CHECK(expected == dst);
	}
	
	rc::prop(
		"sorted_set_union works with arbitrary inputs",
		[](std::set <TestType> const &lhs_, std::set <TestType> const &rhs_){
			
			auto expected_(lhs_);
			
			{
				auto temp(rhs_);
				expected_.merge(temp);
			}
			
			std::vector <TestType> const lhs{lhs_.begin(), lhs_.end()};
			std::vector <TestType> const rhs{rhs_.begin(), rhs_.end()};
			std::vector <TestType> const expected{expected_.begin(), expected_.end()};
			
			REQUIRE(std::is_sorted(lhs.begin(), lhs.end()));
			REQUIRE(std::is_sorted(rhs.begin(), rhs.end()));
			REQUIRE(std::is_sorted(expected.begin(), expected.end()));
			
			std::vector <TestType> dst;
			dst.reserve(expected.size());
			lb::sorted_set_union(lhs, rhs, std::back_inserter(dst));
			
			CHECK(std::is_sorted(dst.begin(), dst.end()));
			CHECK(expected == dst);
			
			return true;
		}
	);
}

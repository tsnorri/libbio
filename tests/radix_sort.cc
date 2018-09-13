/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <algorithm>
#include <catch2/catch.hpp>
#include <libbio/radix_sort.hh>
#include <vector>

namespace gen	= Catch::Generators;
namespace lb	= libbio;


namespace {
	
	struct test
	{
		char c;
		unsigned short s;
		int i;
		unsigned long l;
		long long ll;
	};

	// Since every lambda function has a distinct type, use the visitor pattern to determine the return type size.
	struct lambda_container_base
	{
		virtual ~lambda_container_base() {}
		virtual std::size_t return_type_size() const = 0;
	};
	
	template <typename t_fn>
	struct lambda_container final : public lambda_container_base
	{
		lambda_container(t_fn &&fn_) {} // Only the type is required, not the function.
		std::size_t return_type_size() const override { return lb::detail::return_type_size <t_fn, test>(); }
	};
	
	
	template <typename t_fn>
	std::shared_ptr <lambda_container_base> make_lambda_container(t_fn &&fn)
	{
		return std::shared_ptr <lambda_container_base>(new lambda_container <t_fn>(std::move(fn)));
	}
}


SCENARIO("return_type_size() can return the correct size")
{
	GIVEN("a lambda function")
	{
		auto const functions = GENERATE(gen::table({
			std::make_tuple(sizeof(char),			make_lambda_container([](test const &t) { return t.c; })),
			std::make_tuple(sizeof(unsigned short),	make_lambda_container([](test const &t) { return t.s; })),
			std::make_tuple(sizeof(int),			make_lambda_container([](test const &t) { return t.i; })),
			std::make_tuple(sizeof(unsigned long),	make_lambda_container([](test const &t) { return t.l; })),
			std::make_tuple(sizeof(long long),		make_lambda_container([](test const &t) { return t.ll; }))
		}));
		
		WHEN("the function is called")
		{
			auto const &tup(functions);
			auto const expected_size(std::get <0>(tup));
			auto const determined_size(std::get <1>(tup)->return_type_size());
			
			THEN("the return type size matches the expected size")
			{
				REQUIRE(expected_size == determined_size);
			}
		}
	}
}


SCENARIO("Radix sort can sort a sequence of numbers")
{
	GIVEN("A sequence of numbers")
	{
		auto const vec = GENERATE(gen::values({
			std::vector <unsigned int>({1, 5, 81, 22, 16, 55, 8}),
			std::vector <unsigned int>({55, 12, 74878, 456, 24, 887, 56}),
			std::vector <unsigned int>({123, 3924, 23, 904324, 2320, 99})
		}));
		
		WHEN("the sorting function is called")
		{
			auto v(vec);
			decltype(v) buf;
		
			lb::radix_sort <false>::sort_check_bits_set(v, buf);
			
			THEN("the sequence is sorted")
			{
				REQUIRE(std::is_sorted(v.begin(), v.end()));
			}
		}
	}
}

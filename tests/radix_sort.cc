/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <algorithm>
#include <catch2/catch.hpp>
#include <libbio/radix_sort.hh>
#include <type_traits>
#include <vector>

namespace gen	= Catch::Generators;
namespace lb	= libbio;


namespace {
	
	struct test final
	{
		char c;
		unsigned short s;
		int i;
		unsigned long l;
		long long ll;
	};

	// We would like to test lb::detail::return_type_size().
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
		typedef std::tuple <std::size_t, std::shared_ptr <lambda_container_base>> tuple_type;
		auto const &tuple = GENERATE(table <std::size_t, std::shared_ptr <lambda_container_base>>({
			tuple_type{sizeof(char),			make_lambda_container([](test const &t) { return t.c; })},
			tuple_type{sizeof(unsigned short),	make_lambda_container([](test const &t) { return t.s; })},
			tuple_type{sizeof(int),				make_lambda_container([](test const &t) { return t.i; })},
			tuple_type{sizeof(unsigned long),	make_lambda_container([](test const &t) { return t.l; })},
			tuple_type{sizeof(long long),		make_lambda_container([](test const &t) { return t.ll; })}
		}));
		
		WHEN("the function is called")
		{
			auto const expected_size(std::get <0>(tuple));
			auto const determined_size(std::get <1>(tuple)->return_type_size());
			
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
		typedef std::vector <unsigned int> vector_type;
		auto vec = GENERATE(gen::values({
			vector_type({1, 5, 81, 22, 16, 55, 8}),
			vector_type({55, 12, 74878, 456, 24, 887, 56}),
			vector_type({123, 3924, 23, 904324, 2320, 99})
		}));
		
		WHEN("the sorting function is called")
		{
			vector_type buf;
			lb::radix_sort <false>::sort_check_bits_set(vec, buf);
			
			THEN("the sequence is sorted")
			{
				REQUIRE(std::is_sorted(vec.begin(), vec.end()));
			}
		}
	}
}

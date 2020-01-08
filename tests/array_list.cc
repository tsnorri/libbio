/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/range/combine.hpp>
#include <catch2/catch.hpp>
#include <libbio/array_list.hh>
#include <sstream>

namespace gen	= Catch::Generators;
namespace lb	= libbio;


namespace {
	typedef lb::array_list <int> array_list;
	
	typedef std::vector <
		std::pair <std::size_t, int>
	> vector;
}


SCENARIO("Array list can be instantiated")
{
	GIVEN("Array list items")
	{
		auto const &vec = GENERATE(gen::values({
			vector{{1, 2}, {3, 4}, {5, 6}},
			vector{{2, -1}, {5, -2}, {10, -6}}
		}));
		
		
		WHEN("the collection is instantiated")
		{
			auto il = {vec[0], vec[1], vec[2]};
			array_list list(il);
			
			THEN("the contents match the initializer list")
			{
				for (auto const &pair : il)
					REQUIRE(list[pair.first] == pair.second);
			}
		}
	}
}


#ifdef LIBBIO_BOOST_SERIALIZATION
SCENARIO("Array list can be serialized")
{
	GIVEN("Array list items")
	{
		auto const &il = GENERATE(gen::values({
			initializer_list({{1, 2}, {3, 4}, {5, 6}}),
			initializer_list({{2, -1}, {5, -2}, {10, -6}})
		}));
		
		
		WHEN("the collection is serialized and deserialized")
		{
			array_list list(il);
			
			THEN("the deserialized contents match the original")
			{
				std::stringstream stream;
				array_list list_2;
				
				{
					boost::archive::text_oarchive oa(stream);
					oa << list;
				}
				
				{
					stream.seekg(0);
					boost::archive::text_iarchive ia(stream);
					ia >> list_2;
				}
					
				for (auto const &tup : boost::combine(list.const_pair_iterator_proxy(), list_2.const_pair_iterator_proxy()))
				{
					auto const &lhs(tup.get <0>());
					auto const &rhs(tup.get <1>());
					
					REQUIRE(lhs.first == rhs.first);
					REQUIRE(lhs.second == rhs.second);
				}
			}
		}
	}
}
#endif

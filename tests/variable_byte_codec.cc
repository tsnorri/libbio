/*
 * Copyright (c) 2021-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_template_test_macros.hpp>
#include <iostream>
#include <iterator>
#include <libbio/variable_byte_codec.hh>
#include <limits>
#include <tuple>
#include <vector>

namespace lb	= libbio;


namespace {
	
	template <typename t_encoded_type, typename t_value_type, typename t_maximum_type = t_value_type>
	struct type_specification
	{
		typedef t_encoded_type	encoded_type;
		typedef t_value_type	value_type;
		typedef t_maximum_type	maximum_type;
	};
	
	template <typename t_max_type>
	struct test_1_value_source {};
	
	template <>
	struct test_1_value_source <std::uint8_t>
	{
		template <typename t_type>
		void fill_values(std::vector <t_type> &dst) const
		{
			static_assert(UINT8_MAX <= std::numeric_limits <t_type>::max());
			dst.push_back(0);
			dst.push_back(1);
			dst.push_back(2);
			dst.push_back(16);
			dst.push_back(17);
			dst.push_back(100);
			dst.push_back(200);
			dst.push_back(255);
		}
	};
	
	template <>
	struct test_1_value_source <std::uint16_t>
	{
		template <typename t_type>
		void fill_values(std::vector <t_type> &dst) const
		{
			static_assert(UINT16_MAX <= std::numeric_limits <t_type>::max());
			
			test_1_value_source <std::uint8_t> src;
			src.fill_values(dst);
			
			dst.push_back(256);
			dst.push_back(1000);
			dst.push_back(2000);
			dst.push_back(65000);
			dst.push_back(65535);
		}
	};
	
	template <>
	struct test_1_value_source <std::uint32_t>
	{
		template <typename t_type>
		void fill_values(std::vector <t_type> &dst) const
		{
			static_assert(UINT32_MAX <= std::numeric_limits <t_type>::max());
			
			test_1_value_source <std::uint16_t> src;
			src.fill_values(dst);
			
			dst.push_back(65536);
			dst.push_back(1000000);
			dst.push_back(2000000);
			dst.push_back(0xFFFFFFFF);
		}
	};
	
	template <>
	struct test_1_value_source <std::uint64_t>
	{
		template <typename t_type>
		void fill_values(std::vector <t_type> &dst) const
		{
			static_assert(UINT64_MAX <= std::numeric_limits <t_type>::max());
			
			test_1_value_source <std::uint32_t> src;
			src.fill_values(dst);
			
			dst.push_back(0x100000000UL);
			dst.push_back(0x100000001UL);
			dst.push_back(0x8000000000000000UL);
			dst.push_back(0xFFFFFFFFFFFFFFFFUL);
		}
	};
	
}


TEMPLATE_TEST_CASE(
	"Templated variable byte codec tests",
	"[template]",
	(type_specification <std::uint8_t,	std::uint8_t>),
	(type_specification <std::uint8_t,	std::uint16_t>),
	(type_specification <std::uint8_t,	std::uint32_t>),
	(type_specification <std::uint8_t,	std::uint64_t>),
	(type_specification <std::uint16_t,	std::uint16_t>),
	(type_specification <std::uint16_t,	std::uint32_t>),
	(type_specification <std::uint16_t,	std::uint64_t>),
	(type_specification <std::uint32_t,	std::uint32_t>),
	(type_specification <std::uint32_t,	std::uint64_t>),
	(type_specification <std::uint64_t,	std::uint64_t>),
	(type_specification <std::uint8_t,	std::uint64_t,	std::uint8_t>)
) {
	SECTION("Unsigned integers may be encoded and decoded")
	{
		GIVEN("A list of unsigned integers")
		{
			typedef typename TestType::encoded_type	encoded_type;
			typedef typename TestType::value_type	value_type;
			typedef typename TestType::maximum_type	maximum_type;
			
			std::vector <value_type> values;
			test_1_value_source <maximum_type> value_source;
			value_source.fill_values(values);
			
			WHEN("the integers are encoded")
			{
				lb::variable_byte_codec <encoded_type> codec;
				std::vector <encoded_type> buffer;
				std::back_insert_iterator output_it(buffer);
				
				for (auto const val : values)
					codec.encode(val, output_it);
				
				REQUIRE(values.size() <= buffer.size());
				
				THEN("they can be read back from the stream")
				{
					std::size_t idx{};
					auto it(buffer.cbegin());
					auto const end(buffer.cend());
					while (it != end)
					{
						value_type val{};
						REQUIRE(codec.decode(val, it, end));
						REQUIRE(values[idx] == val);
						++idx;
					}
				}
			}
		}
	}
}

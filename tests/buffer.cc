/*
 * Copyright (c) 2020-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <libbio/buffer.hh>
#include <numeric> // std::iota

namespace gen	= Catch::Generators;
namespace lb	= libbio;


#define BUFFER_VALUE_TYPES \
	std::int8_t, \
	std::int16_t, \
	std::int32_t, \
	std::int64_t, \
	std::uint8_t, \
	std::uint16_t, \
	std::uint32_t, \
	std::uint64_t


namespace {
	
	template <template <typename, typename> typename t_buffer>
	struct buffer_type
	{
		template <typename t_type>
		using buffer_with_copy_contents = t_buffer <t_type, lb::buffer_base::copy_tag>;
		
		template <typename t_type>
		using buffer_with_zero_on_copy = t_buffer <t_type, lb::buffer_base::zero_tag>;
	};
	
	
	template <typename t_buffer, typename t_tested_type>
	struct test_type
	{
		typedef t_buffer		buffer_type;
		typedef t_tested_type	tested_type;
	};
	
	template <typename t_buffer>
	struct buffer_test_type
	{
		template <typename t_tested_type>
		using type = test_type <t_buffer, t_tested_type>;
	};
}


TEMPLATE_PRODUCT_TEST_CASE(
	"buffers can be constructed",
	"[template][buffer][aligned_buffer]",
	(
		buffer_type <lb::buffer>::buffer_with_copy_contents,
		buffer_type <lb::buffer>::buffer_with_zero_on_copy,
		buffer_type <lb::aligned_buffer>::buffer_with_copy_contents,
		buffer_type <lb::aligned_buffer>::buffer_with_zero_on_copy
	),
	(
		BUFFER_VALUE_TYPES
	)
)
{
	SECTION("instantiation")
	{
		GIVEN("a buffer type")
		{
			WHEN("constructed without arguments")
			{
				TestType buffer;
				THEN("the buffer is correctly constructed")
				{
					REQUIRE(buffer.size() == 0);
					REQUIRE(buffer.get() == nullptr);
				}
			}
			
			WHEN("constructed with a size")
			{
				TestType buffer(5);
				THEN("the buffer is correctly constructed")
				{
					REQUIRE(buffer.size() == 5);
					REQUIRE(buffer.get() != nullptr);
				}
			}
		}
	}
}


TEMPLATE_PRODUCT_TEST_CASE(
	"aligned_buffers can be constructed with specific alignment",
	"[template][aligned_buffer]",
	(
		buffer_test_type <lb::aligned_buffer <std::byte, lb::buffer_base::copy_tag>>::template type,
		buffer_test_type <lb::aligned_buffer <std::byte, lb::buffer_base::zero_tag>>::template type
	),
	(
		std::string,
		std::string_view,
		std::vector <std::uint64_t>
	)
)
{
	SECTION("instantiation")
	{
		GIVEN("a buffer type")
		{
			typedef typename TestType::buffer_type	buffer_type;
			typedef typename TestType::tested_type	tested_type;
			constexpr auto const expected_size(sizeof(tested_type));
			constexpr auto const expected_alignment(alignof(tested_type));
			
			static_assert(1 != expected_size);
			static_assert(1 != expected_alignment);
			
			WHEN("constructed with a size and an alignment")
			{
				buffer_type buffer(sizeof(tested_type), alignof(tested_type));
				
				THEN("the buffer is correctly constructed")
				{
					REQUIRE(buffer.size() == expected_size);
					REQUIRE(buffer.alignment() == expected_alignment);
					REQUIRE(buffer.get() != nullptr);
				}
			}
		}
	}
}


TEMPLATE_PRODUCT_TEST_CASE(
	"buffers can be copied",
	"[template][buffer][aligned_buffer]",
	(
		buffer_type <lb::buffer>::buffer_with_copy_contents,
		buffer_type <lb::aligned_buffer>::buffer_with_copy_contents
	),
	(
		BUFFER_VALUE_TYPES
	)
)
{
	SECTION("copying")
	{
		typedef typename TestType::value_type value_type;
		GIVEN("a buffer type")
		{
			constexpr auto const count(5);
			static_assert(count <= std::numeric_limits <value_type>::max());
			TestType src(count);
			
			auto *begin(src.get());
			auto *end(begin + count);
			std::iota(begin, end, value_type(1));
			
			WHEN("a buffer is copied using operator=")
			{
				TestType dst;
				dst = src;
				
				THEN("its contents are copied")
				{
					REQUIRE(src.get() != nullptr);
					REQUIRE(dst.get() != nullptr);
					REQUIRE(dst.size() == count);
					
					for (value_type i(0); i < count; ++i)
					{
						REQUIRE((*src)[i] == (i + 1));
						REQUIRE((*dst)[i] == (i + 1));
					}
				}
			}
			
			WHEN("a buffer is copied using copy constructor")
			{
				TestType dst(src);
				
				THEN("its contents are copied")
				{
					REQUIRE(src.get() != nullptr);
					REQUIRE(dst.get() != nullptr);
					REQUIRE(dst.size() == count);
					
					for (value_type i(0); i < count; ++i)
					{
						REQUIRE((*src)[i] == (i + 1));
						REQUIRE((*dst)[i] == (i + 1));
					}
				}
			}
		}
	}
}


TEMPLATE_PRODUCT_TEST_CASE(
	"buffers can be copied without copying their contents when using zero_on_copy",
	"[template][buffer][aligned_buffer]",
	(
		buffer_type <lb::buffer>::buffer_with_zero_on_copy,
		buffer_type <lb::aligned_buffer>::buffer_with_zero_on_copy
	),
	(
		BUFFER_VALUE_TYPES
	)
)
{
	SECTION("copying")
	{
		typedef typename TestType::value_type value_type;
		GIVEN("a buffer type")
		{
			constexpr auto const count(5);
			static_assert(count <= std::numeric_limits <value_type>::max());
			TestType src(count);
			
			auto *begin(src.get());
			auto *end(begin + count);
			std::iota(begin, end, value_type(1));
			
			WHEN("a buffer is copied using operator=")
			{
				TestType dst;
				dst = src;
				
				THEN("the destination is zeroed")
				{
					REQUIRE(src.get() != nullptr);
					REQUIRE(dst.get() != nullptr);
					REQUIRE(dst.size() == count);
					
					for (value_type i(0); i < count; ++i)
					{
						REQUIRE((*src)[i] == (i + 1));
						REQUIRE((*dst)[i] == 0);
					}
				}
			}
			
			WHEN("a buffer is copied using copy constructor")
			{
				TestType dst(src);
				
				THEN("the destination is zeroed")
				{
					REQUIRE(src.get() != nullptr);
					REQUIRE(dst.get() != nullptr);
					REQUIRE(dst.size() == count);
					
					for (value_type i(0); i < count; ++i)
					{
						REQUIRE((*src)[i] == (i + 1));
						REQUIRE((*dst)[i] == 0);
					}
				}
			}
		}
	}
}


TEMPLATE_PRODUCT_TEST_CASE(
	"buffers can be moved",
	"[template][buffer][aligned_buffer]",
	(
		buffer_type <lb::buffer>::buffer_with_copy_contents,
		buffer_type <lb::buffer>::buffer_with_zero_on_copy,
		buffer_type <lb::aligned_buffer>::buffer_with_copy_contents,
		buffer_type <lb::aligned_buffer>::buffer_with_zero_on_copy
	),
	(
		BUFFER_VALUE_TYPES
	)
)
{
	SECTION("moving")
	{
		typedef typename TestType::value_type value_type;
		GIVEN("a buffer type")
		{
			constexpr auto const count(5);
			static_assert(count <= std::numeric_limits <value_type>::max());
			TestType src(count);
			
			auto *begin(src.get());
			auto *end(begin + count);
			std::iota(begin, end, value_type(1));
			auto const *expected_address(src.get());
			
			WHEN("a buffer is moved using operator=")
			{
				TestType dst;
				dst = std::move(src);
				
				THEN("the underlying buffer is moved")
				{
					REQUIRE(src.get() == nullptr);
					REQUIRE(src.size() == 0);
					REQUIRE(dst.get() == expected_address);
					REQUIRE(dst.size() == count);
					
					for (value_type i(0); i < count; ++i)
						REQUIRE((*dst)[i] == (i + 1));
				}
			}
			
			WHEN("a buffer is moved using a move constructor")
			{
				TestType dst(std::move(src));
				
				THEN("the underlying buffer is moved")
				{
					REQUIRE(src.get() == nullptr);
					REQUIRE(src.size() == 0);
					REQUIRE(dst.get() == expected_address);
					REQUIRE(dst.size() == count);
					
					for (value_type i(0); i < count; ++i)
						REQUIRE((*dst)[i] == (i + 1));
				}
			}
		}
	}
}

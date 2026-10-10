/*
 * Copyright (c) 2026 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <libbio/encoding/elias_gamma.hh>
#include "span_bit_streams.hh"

namespace encoding	= libbio::encoding;
namespace tests		= libbio::tests;


namespace {

	template <typename t_codec, typename t_value>
	void test_elias(t_value value)
	{
		t_codec coder{};
		tests::span_writing_stream writing_stream;
		tests::span_reading_stream reading_stream{writing_stream};

		WHEN("the value is passed to the encoder")
		{
			bool const res{coder.encode(writing_stream.stream, value)};

			THEN("it can be encoded")
			{
				REQUIRE(res == true);
				reading_stream.stream.set_end_position(writing_stream.stream.current_position());
			}
		}

		if constexpr (t_codec::max_encoded_size_is_64_bits)
		{
			if (value <= t_codec::max_value)
			{
				WHEN("the encoded value is decoded by reading it from a word")
				{
					auto const res{coder.decode(writing_stream.buffer.front())};

					THEN("it can be decoded and the decoded value matches the input")
					{
						REQUIRE(bool(res));
						REQUIRE(value == res->first);
						REQUIRE(0 < res->second);
					}
				}
			}
		}

		WHEN("the encoded value is decoded by reading it from a bit stream")
		{
			auto const res{coder.decode(reading_stream.stream)};

			THEN("it can be decoded and the decoded value matches the input")
			{
				REQUIRE(bool(res));
				REQUIRE(value == *res);
			}
		}
	}
}


SCENARIO("Elias gamma encoding works with predefined values as expected", "[elias_gamma]")
{
	GIVEN("std::uint64_t{4294967295}")
	{
		test_elias <encoding::elias_gamma_tpl <false>>(UINT64_C(4294967295));
	}
}

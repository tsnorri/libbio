/*
 * Copyright (c) 2026 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <cstdint>
#include <libbio/bit_reading_stream.hh>
#include <libbio/bit_writing_stream.hh>
#include <libbio/encoding/elias_gamma.hh>
#include <libbio/encoding/elias_delta.hh>
#include <libbio/encoding/zigzag.hh>
#include <libbio/rapidcheck_test_driver.hh>
#include <span>
#include <type_traits>

namespace lb		= libbio;
namespace encoding	= libbio::encoding;


namespace {

	template <typename t_value>
	requires std::is_signed_v <t_value>
	std::make_unsigned_t <t_value> zigzag_naive(t_value const value)
	{
		typedef std::make_unsigned_t <t_value> return_type;

		if (0 <= value)
			return return_type(2U * return_type(value));

		return return_type(-2 * value - 1);
	}


	struct span_writing_stream
	{
		typedef std::span <std::uint64_t, 1> span_type;
		typedef lb::bit_writing_streams::span_target <span_type::extent> target_type;

		std::uint64_t buffer{};
		lb::bit_writing_stream <target_type> stream;

		span_writing_stream():
			stream{
				target_type{
					span_type{&buffer, 1}
				}
			}
		{
		}
	};


	struct span_reading_stream
	{
		lb::bit_reading_stream stream;

		explicit span_reading_stream(span_writing_stream const writing_stream):
			stream{
				writing_stream.stream.target().values()
			}
		{
		}
	};


	template <typename t_codec, typename t_value>
	auto test_elias()
	{
		return lb::rc_check(
			"Elias gamma encoding works as expected",
			[](t_value const value){
				t_codec coder{};
				span_writing_stream writing_stream;
				span_reading_stream reading_stream{writing_stream};

				bool const res{coder.encode(writing_stream.stream, value)};
				RC_ASSERT(res == true);

				auto const res2{coder.decode(writing_stream.buffer)};
				RC_ASSERT(bool(res2));
				RC_ASSERT(value == res2->first);
				RC_ASSERT(0 < res2->second);

				auto const res3{coder.decode(reading_stream.stream)};
				RC_ASSERT(bool(res3));
				RC_ASSERT(value == *res3);
			}
		);
	}
}


TEMPLATE_TEST_CASE(
	"Zigzag encoding works as expected",
	"[template][encoding::zigzag]",
	std::int8_t,
	std::int16_t,
	std::int32_t,
	std::int64_t
)
{
	return lb::rc_check(
		"Zigzag encoding works as expected",
		[](TestType const value){
			encoding::zigzag coder{};

			auto const expected_encoded{zigzag_naive(value)};
			auto const actual_encoded{coder.encode(value)};
			RC_ASSERT(expected_encoded == actual_encoded);

			auto const actual_decoded{coder.decode(actual_encoded)};
			RC_ASSERT(value == actual_decoded);
		}
	);
}


TEMPLATE_TEST_CASE(
	"Elias gamma encoding works as expected",
	"[template][encoding::elias_gamma]",
	std::int8_t,
	std::int16_t,
	std::int32_t,
	std::int64_t
)
{
	return test_elias <encoding::elias_gamma, TestType>();
}


TEMPLATE_TEST_CASE(
	"Elias delta encoding works as expected",
	"[template][encoding::elias_delta]",
	std::int8_t,
	std::int16_t,
	std::int32_t,
	std::int64_t
)
{
	return test_elias <encoding::elias_delta, TestType>();
}

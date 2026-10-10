/*
 * Copyright (c) 2026 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <cstdint>
#include <libbio/algorithm.hh>
#include <libbio/bit_reading_stream.hh>
#include <libbio/bit_writing_stream.hh>
#include <libbio/encoding/elias_gamma.hh>
#include <libbio/encoding/elias_delta.hh>
#include <libbio/encoding/zigzag.hh>
#include <libbio/rapidcheck/closed_range.hh>
#include <libbio/rapidcheck_test_driver.hh>
#include <limits>
#include <type_traits>
#include "integer_type_name.hh"
#include "span_bit_streams.hh"

namespace lb		= libbio;
namespace encoding	= libbio::encoding;
namespace tests		= libbio::tests;


namespace {

	template <typename t_type, typename t_codec>
	struct test_input
	{
		typedef t_type value_type;
		typedef t_codec codec_type;

		value_type value{};
	};


	template <typename t_value>
	requires std::is_signed_v <t_value>
	std::make_unsigned_t <t_value> zigzag_naive(t_value const value)
	{
		typedef std::make_unsigned_t <t_value> return_type;

		if (0 <= value)
			return return_type(2U * return_type(value));

		return return_type(-2 * value - 1);
	}


	template <typename t_test_type>
	auto test_elias()
	{
		return lb::rc_check(
			"Elias gamma encoding works as expected",
			[](t_test_type const input){
				typedef typename t_test_type::value_type value_type;
				typedef typename t_test_type::codec_type codec_type;

				RC_LOG() << "Type: " << tests::integer_type_name <value_type>::value << '\n';
				RC_LOG() << "Value: " << +input.value << '\n';

				codec_type coder{};
				tests::span_writing_stream writing_stream;
				tests::span_reading_stream reading_stream{writing_stream};

				{
					bool const res{coder.encode(writing_stream.stream, input.value)};
					RC_ASSERT(res == true);
					reading_stream.stream.set_end_position(writing_stream.stream.current_position());
				}

				if constexpr (codec_type::max_encoded_size_is_64_bits)
				{
					if (input.value <= codec_type::max_value)
					{
						auto const res{coder.decode(writing_stream.buffer.front())};
						RC_ASSERT(bool(res));
						RC_ASSERT(input.value == res->first);
						RC_ASSERT(0 < res->second);
					}
				}

				{
					auto const res{coder.decode(reading_stream.stream)};
					RC_ASSERT(bool(res));
					RC_ASSERT(input.value == *res);
				}
			}
		);
	}
}


namespace rc {

	template <typename t_type, typename t_codec>
	struct Arbitrary <test_input <t_type, t_codec>>
	{
		static Gen <test_input <t_type, t_codec>> arbitrary()
		{
			constexpr auto const max{lb::min_ct(
				t_codec::max_value,
				std::numeric_limits <t_type>::max()
			)};

			return gen::construct <test_input <t_type, t_codec>>(
				gen::inClosedRange(t_type{}, t_type{max})
			);
		}
	};
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
	(test_input <std::uint8_t, encoding::elias_gamma>),
	(test_input <std::uint16_t, encoding::elias_gamma>),
	(test_input <std::uint32_t, encoding::elias_gamma>),
	(test_input <std::uint64_t, encoding::elias_gamma>),
	(test_input <std::uint64_t, encoding::elias_gamma_tpl <false>>)
)
{
	return test_elias <TestType>();
}


TEMPLATE_TEST_CASE(
	"Elias delta encoding works as expected",
	"[template][encoding::elias_delta]",
	(test_input <std::uint8_t, encoding::elias_delta>),
	(test_input <std::uint16_t, encoding::elias_delta>),
	(test_input <std::uint32_t, encoding::elias_delta>),
	(test_input <std::uint64_t, encoding::elias_delta>),
	(test_input <std::uint64_t, encoding::elias_delta_tpl <false>>)
)
{
	return test_elias <TestType>();
}

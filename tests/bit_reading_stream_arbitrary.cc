/*
 * Copyright (c) 2026 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <libbio/bit_reading_stream.hh>
#include <libbio/bits.hh>
#include <libbio/rapidcheck_test_driver.hh>
#include <vector>
#include "partial_word.hh"

namespace lb		= libbio;
namespace tests		= lb::tests;


TEST_CASE(
	"bit_reading_stream works as expected",
	"[template][bit_reading_stream]",
)
{
	return lb::rc_check(
		"bit_reading_stream works as expected",
		[](std::vector <tests::partial_word> const &input){
			typedef lb::bit_reading_stream::value_type value_type;

			// Prepare source.
			std::vector <tests::partial_word::word_type> source;
			auto const bit_size{tests::write_to_buffer(input, source)};
			lb::bit_reading_stream stream{source, 0, bit_size};

			// Check.
			for (tests::partial_word const pw : input)
			{
				value_type const mask{~((~(value_type{})) << pw.bit_count)};
				auto const value{stream.value() & mask};
				RC_ASSERT(pw.word == value);
				stream >>= pw.bit_count;
			}
		}
	);
}

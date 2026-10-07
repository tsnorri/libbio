/*
 * Copyright (c) 2026 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <libbio/bits.hh>
#include <libbio/bit_writing_stream.hh>
#include <libbio/rapidcheck_test_driver.hh>
#include <type_traits>
#include <vector>
#include "partial_word.hh"

namespace lb		= libbio;
namespace tests		= lb::tests;


TEMPLATE_TEST_CASE(
	"bit_writing_stream works as expected",
	"[template][bit_writing_stream]",
	std::false_type,
	std::true_type
)
{
	return lb::rc_check(
		"bit_writing_stream works as expected",
		[](std::vector <tests::partial_word> const &input){
			// Prepare expected.
			std::vector <tests::partial_word::word_type> expected;
			tests::write_to_buffer(input, expected);

			// Prepare actual.
			lb::bit_writing_stream stream{lb::bit_writing_streams::vector_target{}};
			for (tests::partial_word const pw : input)
			{
				if constexpr (TestType::value)
				{
					if (pw.word)
						RC_ASSERT(stream.write_bits(pw.word, pw.bit_count));
					else
						RC_ASSERT(stream.write_zeros(pw.bit_count));
				}
				else
				{
					RC_ASSERT(stream.write_bits(pw.word, pw.bit_count));
				}
			}

			// Compare.
			RC_ASSERT(expected == stream.target().values());
		}
	);
}

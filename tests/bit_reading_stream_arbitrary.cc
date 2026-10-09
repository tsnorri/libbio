/*
 * Copyright (c) 2026 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <libbio/bit_reading_stream.hh>
#include <libbio/bits.hh>
#include <libbio/rapidcheck_test_driver.hh>
#include <format>
#include <range/v3/view/enumerate.hpp>
#include <span>
#include <vector>
#include "partial_word.hh"

namespace lb		= libbio;
namespace rsv		= ranges::views;
namespace tests		= lb::tests;


TEST_CASE(
	"bit_reading_stream works as expected",
	"[bit_reading_stream]"
)
{
	return lb::rc_check(
		"bit_reading_stream works as expected",
		[](std::vector <tests::partial_word> const &input){
			// Prepare source.
			std::vector <tests::partial_word::word_type> source;
			auto const bit_size{tests::write_to_buffer(input, source, true)};
			lb::bit_reading_stream stream{source, 0, bit_size};

			{
				std::span <char const> source_{
					reinterpret_cast<char const *>(source.data()),
					source.size() * sizeof(tests::partial_word::word_type)
				};
				RC_LOG() << "source:";
				for (auto const word : source_)
					RC_LOG() << std::format(" {:02X}", word);
				RC_LOG() << '\n';
			}

			// Check.
			for (auto const [idx, pw] : rsv::enumerate(input))
			{
				RC_LOG() << "idx: " << idx << '\n';
				auto const value{stream.read_bits(pw.bit_count)};
				RC_ASSERT(pw.word == value);
				stream >>= pw.bit_count;
			}
		}
	);
}

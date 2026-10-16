/*
 * Copyright (c) 2026 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <cstdint>
#include <libbio/bit_reading_stream.hh>
#include <libbio/bits.hh>
#include <libbio/rapidcheck_test_driver.hh>
#include <libbio/rapidcheck/closed_range.hh>
#include <format>
#include <ostream>
#include <range/v3/view/enumerate.hpp>
#include <span>
#include <vector>
#include "partial_word.hh"

namespace lb		= libbio;
namespace rsv		= ranges::views;
namespace tests		= lb::tests;


namespace {

	struct shiftable_test_input
	{
		typedef std::uint64_t value_type;
		typedef std::uint8_t shift_amount_type;
		constexpr static inline shift_amount_type const value_bits{CHAR_BIT * sizeof(value_type)};

		value_type value{};
		shift_amount_type shift_amount{};
	};


	std::ostream &operator<<(std::ostream &os, shiftable_test_input const input)
	{
		os << "value: " << input.value << " shift_amount: " << +input.shift_amount;
		return os;
	}


	lb::bit_reading_stream make_stream(std::uint64_t &value)
	{
		return {
			std::span <std::uint64_t, 1>{&value, 1},
			0,
			64
		};
	};
}


namespace rc {

	template <>
	struct Arbitrary <shiftable_test_input>
	{
		static Gen <shiftable_test_input> arbitrary()
		{
			typedef shiftable_test_input::value_type value_type;
			typedef shiftable_test_input::shift_amount_type shift_amount_type;

			constexpr shift_amount_type const max_shift{shiftable_test_input::value_bits - 1};

			return gen::construct <shiftable_test_input>(
				gen::arbitrary <value_type>(),
				gen::inClosedRange(shift_amount_type{}, max_shift)
			);
		}
	};
}


TEST_CASE(
	"bit_reading_stream::copy_to() works as expected",
	"[bit_reading_stream]"
)
{
	return lb::rc_test(
		[](shiftable_test_input input){
			typedef shiftable_test_input::value_type value_type;

			value_type buffer{input.value};
			auto bs{make_stream(buffer)};

			bs >>= input.shift_amount;
			input.value >>= input.shift_amount;

			auto const actual{bs.read_bits(input.value_bits - input.shift_amount)};
			RC_ASSERT(input.value == actual);
		}
	);
}


TEST_CASE(
	"bit_reading_stream::operator>>= works as expected",
	"[bit_reading_stream]"
)
{
	return lb::rc_test(
		[](shiftable_test_input input){
			typedef shiftable_test_input::value_type value_type;

			value_type buffer{input.value};
			auto bs{make_stream(buffer)};

			bs >>= input.shift_amount;
			input.value >>= input.shift_amount;

			value_type actual{};
			bs.copy_to(std::span <value_type, 1>{&actual, 1});
			RC_ASSERT(input.value == actual);
		}
	);
}


TEST_CASE(
	"Ranges of bits can be read from bit_reading_stream",
	"[bit_reading_stream]"
)
{
	return lb::rc_test(
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

/*
 * Copyright (c) 2026 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <cstdint>
#include <libbio/bits.hh>
#include <numeric>
#include <ranges>
#include <span>
#include <tuple>
#include <type_traits>
#include <vector>
#include "partial_word.hh"


namespace libbio::tests {

	std::uint64_t write_to_buffer(
		std::vector <partial_word> const &src,
		std::vector <partial_word::word_type> &dst,
		bool const should_reverse
	)
	{
		auto const total_bit_size{
			std::accumulate(
				src.begin(),
				src.end(),
				std::uint64_t{},
				[](auto const acc, partial_word const range){
					return acc + range.bit_count;
				}
			)
		};

		dst.clear();
		dst.resize((total_bit_size + (partial_word::word_bits - 1U)) / partial_word::word_bits, 0);
		if (should_reverse)
		{
			for (partial_word const pw : std::ranges::reverse_view(src))
			{
				libbio::bits::shift_span_left(std::span{dst}, pw.bit_count, std::true_type{});
				dst.front() |= pw.word;
			}
		}
		else
		{
			for (partial_word const pw : src)
			{
				libbio::bits::shift_span_left(std::span{dst}, pw.bit_count, std::true_type{});
				dst.front() |= pw.word;
			}
		}

		return total_bit_size;
	}
}


namespace rc {

	auto Arbitrary <libbio::tests::partial_word>::arbitrary() -> Gen <libbio::tests::partial_word>
	{
		typedef libbio::tests::partial_word::word_type word_type;
		typedef libbio::tests::partial_word::bit_count_type bit_count_type;

		return gen::map(
			gen::tuple(
				gen::arbitrary <word_type>(),
				gen::inRange(bit_count_type{1}, bit_count_type{65})
			),
			[](std::tuple <word_type, bit_count_type> args) -> libbio::tests::partial_word {
				auto &[word, bit_count]  = args;
				word_type const mask{~((~(word_type{})) << bit_count)};
				word &= mask;
				RC_ASSERT(0 == word >> bit_count);
				return {word, bit_count};
			}
		);
	}
}

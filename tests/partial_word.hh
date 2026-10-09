/*
 * Copyright (c) 2026 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_TESTS_PARTIAL_WORD_HH
#define LIBBIO_TESTS_PARTIAL_WORD_HH

#include <climits>
#include <cstdint>
#include <format>
#include <libbio/rapidcheck_test_driver.hh>
#include <ostream>
#include <vector>


namespace libbio::tests {

	struct partial_word
	{
		typedef std::uint64_t word_type;
		typedef std::uint8_t bit_count_type;
		constexpr static inline word_type const word_bits{sizeof(word_type) * CHAR_BIT};

		word_type word{};
		bit_count_type bit_count{};
	};


	std::uint64_t write_to_buffer(
		std::vector <partial_word> const &src,
		std::vector <partial_word::word_type> &dst,
		bool should_reverse = false
	);


	inline std::ostream &operator<<(std::ostream &os, partial_word const pw)
	{
		os << std::format("(w: {:02X} b: {})", pw.word, +pw.bit_count);
		return os;
	}
}


namespace rc {

	template <>
	struct Arbitrary <libbio::tests::partial_word>
	{
		static Gen <libbio::tests::partial_word> arbitrary();
	};
}

#endif

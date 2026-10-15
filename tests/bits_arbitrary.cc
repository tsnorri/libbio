/*
 * Copyright (c) 2026 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <libbio/bits.hh>
#include <libbio/rapidcheck_test_driver.hh>
#include <ostream>
#include <span>
#include <type_traits>

namespace lb	= libbio;


namespace {

	typedef std::uint64_t input_value_type;


	template <typename t_word>
	struct input_buffer {};

	template <> struct input_buffer <std::uint64_t>
	{
		constexpr static inline std::uint8_t const element_count{1};
		std::uint64_t value[1]{};
	};

	template <> struct input_buffer <std::uint32_t>
	{
		constexpr static inline std::uint8_t const element_count{2};
		std::uint32_t value[2]{};
	};

	template <> struct input_buffer <std::uint16_t>
	{
		constexpr static inline std::uint8_t const element_count{4};
		std::uint16_t value[4]{};
	};

	template <> struct input_buffer <std::uint8_t>
	{
		constexpr static inline std::uint8_t const element_count{8};
		std::uint8_t value[8]{};
	};


	template <typename t_word, bool t_should_shift_multiple_elements>
	struct test_input
	{
		typedef t_word word_type;
		typedef input_buffer <t_word> buffer_type;
		typedef std::span <word_type, buffer_type::element_count> span_type;
		typedef std::span <word_type const, buffer_type::element_count> const_span_type;

		constexpr static inline std::uint8_t word_bits{CHAR_BIT * sizeof(word_type)};
		constexpr static inline bool should_shift_multiple_elements{t_should_shift_multiple_elements};

		static_assert(sizeof(word_type) <= sizeof(input_value_type));
		static_assert(0 == sizeof(input_value_type) % sizeof(word_type));

		buffer_type buffer_{};
		std::uint8_t shift_amount_{};

		span_type value_span() { return span_type{buffer_.value}; }
		const_span_type value_span() const { return const_span_type{buffer_.value}; }

		test_input(std::uint64_t const val, std::uint8_t const shift_amount):
			shift_amount_{shift_amount}
		{
			memcpy(buffer_.value, &val, sizeof(std::uint64_t));
		}

		std::uint64_t value() const
		{
			std::uint64_t retval{};
			memcpy(&retval, buffer_.value, sizeof(std::uint64_t));
			return retval;
		}
	};


	template <typename t_word, bool t_should_shift_multiple_elements>
	std::ostream &operator<<(std::ostream &os, test_input <t_word, t_should_shift_multiple_elements> const input)
	{
		os << "value_: " << input.value()
			<< " shift_amount_: " << (+input.shift_amount_)
			<< " should_shift_multiple: " << t_should_shift_multiple_elements
			<< " span:";

		for (auto const vv : input.value_span())
			os << std::format(" {:02X}", vv);

		return os;
	}


	enum class shift_direction
	{
		left,
		right
	};

	template <shift_direction t_direction, bool t_should_shift_multiple_elements>
	struct shift_
	{
	};

	template <bool t_should_shift_multiple_elements>
	struct shift_ <shift_direction::left, t_should_shift_multiple_elements>
	{
		void operator()(input_value_type &value, std::uint8_t const amount) const
		{
			value <<= amount;
		}

		template <typename t_word, std::size_t t_extent>
		void operator()(std::span <t_word, t_extent> span, std::uint8_t const amount) const
		{
			lb::bits::shift_span_left(span, amount, std::bool_constant <t_should_shift_multiple_elements>{});
		}
	};

	template <bool t_should_shift_multiple_elements>
	struct shift_ <shift_direction::right, t_should_shift_multiple_elements>
	{
		void operator()(input_value_type &value, std::uint8_t const amount) const
		{
			value >>= amount;
		}

		template <typename t_word, std::size_t t_extent>
		void operator()(std::span <t_word, t_extent> span, std::uint8_t const amount) const
		{
			lb::bits::shift_span_right(span, amount, std::bool_constant <t_should_shift_multiple_elements>{});
		}
	};


	template <typename t_word, bool t_should_shift_multiple_elements, shift_direction t_direction>
	void test_(
		test_input <t_word, t_should_shift_multiple_elements> input,
		shift_ <t_direction, t_should_shift_multiple_elements> shift
	)
	{
		auto value{input.value()};
		auto span{input.value_span()};

		shift(value, input.shift_amount_);
		shift(span, input.shift_amount_);
		RC_ASSERT(value == input.value());
	}


	template <typename t_word>
	using test_span_less_than_one_word = test_input <t_word, false>;

	template <typename t_word>
	using test_span_more_than_one_word = test_input <t_word, true>;
}


namespace rc {

	template <typename t_word, bool t_should_shift_multiple_elements>
	struct Arbitrary <test_input <t_word, t_should_shift_multiple_elements>>
	{
		static Gen <test_input <t_word, t_should_shift_multiple_elements>> arbitrary()
		{
			typedef test_input <t_word, t_should_shift_multiple_elements> return_type;

			constexpr std::uint8_t const min_shift{
				return_type::should_shift_multiple_elements
				? return_type::word_bits
				: 0
			};

			constexpr std::uint8_t const max_shift{
				return_type::should_shift_multiple_elements
				? 63
				: return_type::word_bits - 1
			};

			return gen::construct <return_type>(
				gen::arbitrary <input_value_type>(),
				gen::inRange(min_shift, max_shift)
			);
		}
	};
}


TEMPLATE_PRODUCT_TEST_CASE(
	"bits::shift_span_left works as expected",
	"[template]",
	(
		test_span_less_than_one_word,
		test_span_more_than_one_word
	),
	(
		std::uint8_t,
		std::uint16_t,
		std::uint32_t
	)
)
{
	typedef shift_ <
		shift_direction::left,
		TestType::should_shift_multiple_elements
	> shift_type;

	return lb::rc_test(
		[](TestType input){
			test_(input, shift_type{});
		}
	);
}


TEMPLATE_PRODUCT_TEST_CASE(
	"bits::shift_span_right works as expected",
	"[template]",
	(
		test_span_less_than_one_word,
		test_span_more_than_one_word
	),
	(
		std::uint8_t,
		std::uint16_t,
		std::uint32_t
	)
)
{
	typedef shift_ <
		shift_direction::right,
		TestType::should_shift_multiple_elements
	> shift_type;

	return lb::rc_test(
		[](TestType input){
			test_(input, shift_type{});
		}
	);
}

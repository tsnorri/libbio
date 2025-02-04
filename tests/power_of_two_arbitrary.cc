/*
 * Copyright (c) 2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <climits>
#include <cstdint>
#include <libbio/bits.hh>
#include <libbio/rapidcheck_test_driver.hh>

namespace lb		= libbio;

namespace {

	template <typename t_type>
	struct input
	{
		typedef t_type type;
		t_type value{};

		constexpr t_type get() const { return value; }
		constexpr operator t_type() const { return value; }
	};


	template <typename t_type>
	struct valid_input : public input <t_type> {};

	template <typename t_type>
	struct invalid_input : public input <t_type> {};


	template <typename t_type>
	constexpr t_type highest_bit_set()
	{
		return t_type(1) << (sizeof(t_type) * CHAR_BIT - 1);
	}
}


namespace rc {

	template <typename t_type>
	struct Arbitrary <valid_input <t_type>>
	{
		static Gen <valid_input <t_type>> arbitrary()
		{
			return gen::construct <valid_input <t_type>>(
				gen::inRange(t_type(0), highest_bit_set <t_type>())
			);
		}
	};


	template <typename t_type>
	struct Arbitrary <invalid_input <t_type>>
	{
		static Gen <invalid_input <t_type>> arbitrary()
		{
			return gen::construct <invalid_input <t_type>>(
				gen::inRange(t_type(highest_bit_set <t_type>() + 1), std::numeric_limits <t_type>::max())
			);
		}
	};
}


TEMPLATE_TEST_CASE(
	"bits::gte_power_of_2 works with arbitrary valid input",
	"[template][gte_power_of_2]",
	valid_input <std::uint8_t>,
	valid_input <std::uint16_t>,
	valid_input <std::uint32_t>,
	valid_input <std::uint64_t>
)
{
	return lb::rc_check(
		"bits::is_power_of_2 works with arbitrary valid input",
		[](TestType const value){
			auto const value_(value.get());
			auto const power(lb::bits::gte_power_of_2(value_));
			RC_ASSERT(power);
			RC_ASSERT(value_ <= power);
			RC_ASSERT(lb::bits::is_power_of_2(power));
			RC_CLASSIFY(lb::bits::is_power_of_2(value_));
			return true;
		}
	);
}


TEMPLATE_TEST_CASE(
	"bits::gte_power_of_2 works with arbitrary invalid input",
	"[template][gte_power_of_2]",
	invalid_input <std::uint8_t>,
	invalid_input <std::uint16_t>,
	invalid_input <std::uint32_t>,
	invalid_input <std::uint64_t>
)
{
	return lb::rc_check(
		"bits::is_power_of_2 works with arbitrary invalid input",
		[](TestType const value){
			auto const value_(value.get());
			auto const power(lb::bits::gte_power_of_2(value_));
			RC_ASSERT(!power);
			return true;
		}
	);
}

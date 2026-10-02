/*
 * Copyright (c) 2026 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_ENCODING_ZIGZAG_HH
#define LIBBIO_ENCODING_ZIGZAG_HH

#include <bit>
#include <climits>
#include <type_traits>


namespace libbio::encoding {

	struct zigzag
	{
		template <typename t_integer>
		requires std::is_signed_v <t_integer>
		static std::make_unsigned_t <t_integer> encode(t_integer value)
		{
			typedef t_integer signed_type;
			typedef std::make_unsigned_t <t_integer> unsigned_type;
			constexpr signed_type const shift_amt{CHAR_BIT * sizeof(signed_type) - 1U};

			// Since C++20, “right-shift on signed integral types is an arithmetic right
			// shift, which performs sign-extension.”
			signed_type const multiplied{value << 1};
			signed_type const mask{value >> shift_amt};
			signed_type const retval{multiplied ^ mask};
			return std::bit_cast <unsigned_type>(retval);
		}


		template <typename t_integer>
		requires std::is_unsigned_v <t_integer>
		static std::make_signed_t <t_integer> decode(t_integer encoded)
		{
			typedef t_integer unsigned_type;
			typedef std::make_signed_t <t_integer> signed_type;
			constexpr signed_type const shift_amt{CHAR_BIT * sizeof(signed_type) - 1U};
			constexpr signed_type const sign_mask{std::bit_cast <signed_type>(std::rotr(unsigned_type{1}, 1))};
			constexpr unsigned_type const value_mask{(~(unsigned_type{})) >> 1U};

			auto const sign_bit{std::bit_cast <signed_type>(std::rotr(encoded, 1)) & sign_mask};
			auto const xor_mask{sign_bit >> shift_amt};
			auto const value{encoded ^ xor_mask};
			auto const value_{std::bit_cast <signed_type>((value & value_mask) >> 1U)};
			auto const value__{sign_bit | value_};
			return value__;
		}
	};
}

#endif

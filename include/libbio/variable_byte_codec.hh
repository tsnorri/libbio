/*
 * Copyright (c) 2021 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VARIABLE_BYTE_CODEC_HH
#define LIBBIO_VARIABLE_BYTE_CODEC_HH

#include <boost/endian/conversion.hpp>
#include <type_traits>

namespace libbio { namespace detail {
	
	struct variable_byte_codec_iterator_adapter
	{
		inline constexpr static bool has_value_type{true};
		
		template <typename t_iterator>
		using value_type = typename std::remove_reference_t <t_iterator>::value_type;
		
		template <typename t_input_iterator, typename t_value>
		void read(t_input_iterator &it, t_value &val) const
		{
			typedef typename t_input_iterator::value_type input_value_type;
			static_assert(std::numeric_limits <input_value_type>::max() <= std::numeric_limits <t_value>::max());
			val = *it;
		}
		
		template <typename t_output_iterator, typename t_value>
		void write(t_output_iterator &it, t_value const val) const { *it = val; }
		
		template <typename t_iterator>
		void advance(t_iterator &it) const { ++it; }
		
		template <typename t_iterator, typename t_end_iterator>
		bool can_continue(t_iterator const &it, t_end_iterator const &end) const { return it != end; }
	};
	
	
	struct variable_byte_codec_functor_adapter
	{
		inline constexpr static bool has_value_type{false};
		
		template <typename t_functor, typename t_value>
		void read(t_functor &&fn, t_value &val) const { fn(val); }
		
		template <typename t_functor, typename t_value>
		void write(t_functor &&fn, t_value const val) const { fn(val); }
		
		template <typename t_functor>
		void advance(t_functor &&) const {}
	};
}}


namespace libbio {
	
	// Encode non-negtive integers with a variable number of bytes or words.
	// Sp. the target unit t_word has B bits.
	//
	// Encoding:
	// 1. Convert the value to big endian.
	// 2. Store the lowest B - 1 bits.
	// 3. Shift the value to the right by B - 1. If the value is non-zero, set the highest bit of the stored value to 1 and repeat from 2.
	//
	// Decoding:
	// 1. Read the lowest B - 1 bits.
	// 2. Check the highest bit. If it is zero, stop. Otherwise read the next B - 1 bits from the next byte.
	
	
	template <typename t_encoded, typename t_io_adapter, bool t_convert_endianness>
	class variable_byte_codec_tpl
	{
	public:
		typedef t_encoded encoded_type;
		inline constexpr static encoded_type ENCODED_VALUE_MASK{std::numeric_limits <encoded_type>::max() >> 1};
		inline constexpr static encoded_type ENCODED_VALUE_BITS{CHAR_BIT * sizeof(encoded_type) - 1};
		inline constexpr static encoded_type ENCODED_TYPE_HIGHEST_BIT_MASK{encoded_type(1) << ENCODED_VALUE_BITS};
		
		static_assert(std::is_unsigned_v <encoded_type>);
		static_assert(1 < CHAR_BIT * sizeof(encoded_type));
		static_assert(ENCODED_VALUE_MASK);
		static_assert(ENCODED_VALUE_BITS);
		static_assert(ENCODED_TYPE_HIGHEST_BIT_MASK);
		static_assert(0 == encoded_type(ENCODED_TYPE_HIGHEST_BIT_MASK << 1));
		
		
	protected:
		/* [[no_unique_address]] */ t_io_adapter m_io_adapter{};
		
		
	public:
		template <typename t_src, typename t_output>
		void encode(t_src src_, t_output &&output) const
		{
			static_assert(std::is_unsigned_v <t_src>);
			
			typedef std::common_type_t <t_src, encoded_type> src_type;
			
			// Check that the right-shift below does not cause undefined behaviour.
			static_assert(ENCODED_VALUE_BITS < CHAR_BIT * sizeof(src_type));
			
			src_type src(src_);
			while (true)
			{
				encoded_type enc(src & ENCODED_VALUE_MASK);
				src >>= ENCODED_VALUE_BITS;
				if (src)
				{
					enc |= ENCODED_TYPE_HIGHEST_BIT_MASK;
					m_io_adapter.write(output, t_convert_endianness ? boost::endian::native_to_big(enc) : enc);
					m_io_adapter.advance(output);
				}
				else
				{
					m_io_adapter.write(output, t_convert_endianness ? boost::endian::native_to_big(enc) : enc);
					m_io_adapter.advance(output);
					break;
				}
			}
		}
		
		
		template <typename t_dst, typename t_input, typename t_can_continue_fn>
		bool decode_with_check(t_dst &dst_, t_input &&input, t_can_continue_fn &&can_continue) const
		{
			static_assert(std::is_unsigned_v <t_dst>);
			static_assert(!t_io_adapter::has_value_type || std::is_same_v <typename t_io_adapter::template value_type <t_input>, encoded_type>);
			
			std::size_t shift_amt{};
			t_dst dst{};
			while (true)
			{
				if (!can_continue())
					return false;
				
				// Read a byte or a word.
				encoded_type current_value{};
				m_io_adapter.read(input, current_value);
				m_io_adapter.advance(input);
				if constexpr (t_convert_endianness)
					current_value = boost::endian::big_to_native(current_value);
				
				dst |= t_dst(current_value & ENCODED_VALUE_MASK) << shift_amt;
				auto const has_more_bits(current_value & ENCODED_TYPE_HIGHEST_BIT_MASK);
				if (has_more_bits)
				{
					shift_amt += ENCODED_VALUE_BITS;
					
					// Sanity check.
					if (CHAR_BIT * sizeof(t_dst) <= shift_amt)
						return false;
				}
				else
				{
					dst_ = dst;
					return true;
				}
			}
		}
		
		
		template <typename t_dst, typename t_input, typename t_input_end>
		bool decode(t_dst &dst_, t_input &&input, t_input_end const &input_end) const
		{
			static_assert(std::is_same_v <std::decay_t <t_input>, std::decay_t <t_input_end>>);
			return decode_with_check(
				dst_,
				std::forward <t_input>(input),
				[this, &input, &input_end](){ return m_io_adapter.can_continue(input, input_end); }
			);
		}
		
		
		template <typename t_dst, typename t_input>
		bool decode(t_dst &dst_, t_input &&input) const
		{
			return decode_with_check(
				dst_,
				std::forward <t_input>(input),
				[]() constexpr -> bool { return true; }
			);
		}
	};
	
	
	template <typename t_encoded>
	using variable_byte_codec = variable_byte_codec_tpl <t_encoded, detail::variable_byte_codec_iterator_adapter, true>;
	
	template <typename t_encoded>
	using variable_byte_archiver = variable_byte_codec_tpl <t_encoded, detail::variable_byte_codec_functor_adapter, false>;
}

#endif

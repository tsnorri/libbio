/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_BIT_RLE_CODEC_HH
#define LIBBIO_BIT_RLE_CODEC_HH

#include <boost/endian/conversion.hpp>
#include <boost/iostreams/concepts.hpp>
#include <iostream>


namespace libbio { namespace detail {
}}


namespace libbio {
	
	// Use an RLE encoding for runs of bits.
	// Sp. t_word has B bits.
	//
	// Decoding:
	// 1. Read word w1 from the stream, convert to native endianness. The value is determined by doing w1 >> (B - 1).
	// 2. Read words w2, w3… from the stream until the highest bit of the word changes or the total amount of count bits exceeds 64 by reading the next word.
	// 3. Calculate the count value: set the highest bit of w1, w2, w3… to zero. Then do (w1 | (w2 << (B - 1)) | (w3 << 2 * (B - 1)) | …).
	//
	// Encoding:
	// Given count c and value b, take the lowest (B - 1) bits of c, set the highest bit of the resulting word to b, convert to big endian and write to the stream.
	// Repeat while there are still set bits in c.
	//
	// FIXME: This could be improved by having more than two word formats: another one for storing the run (e.g. next 63 bits) instead of the value and the count into the word.
	
	template <typename t_word>
	class bit_rle_decoder
	{
	protected:
		typedef std::istream	stream_type;
		static_assert(alignof(stream_type::char_type) <= alignof(t_word));
		
		enum : std::uint64_t {
			WORD_BITS	= sizeof(t_word) * CHAR_BIT,						// Number of bits in t_word.
			COUNT_BITS	= WORD_BITS - 1,									// Number of bits for count value in t_word.
			COUNT_MASK	= std::numeric_limits <t_word>::max() >> 1,
			READ_SIZE	= sizeof(t_word) / sizeof(stream_type::char_type)
		};
		
	protected:
		stream_type		*m_stream{};
		t_word			m_next_word{};
		bool			m_have_next{};
		
	public:
		bit_rle_decoder(stream_type &stream):
			m_stream(&stream)
		{
		}
		
		bool prepare();
		bool read_next_run(bool &value, std::uint64_t &count);
		
	protected:
		bool read_next_word();
	};
	
	
	template <typename t_word>
	class bit_rle_encoder
	{
	protected:
		typedef std::ostream	stream_type;
		static_assert(alignof(stream_type::char_type) <= alignof(t_word));
		
		enum : std::uint64_t {
			WORD_BITS	= sizeof(t_word) * CHAR_BIT,						// Number of bits in t_word.
			COUNT_BITS	= WORD_BITS - 1,									// Number of bits for count value in t_word.
			COUNT_MASK	= std::numeric_limits <t_word>::max() >> 1,
			WRITE_SIZE	= sizeof(t_word) / sizeof(stream_type::char_type)
		};
		
	protected:
		stream_type	*m_stream{};
		
	public:
		bit_rle_encoder(stream_type &stream):
			m_stream(&stream)
		{
		}

		void write_run(bool value, std::uint64_t count);
	};
	
	
	template <typename t_word>
	bool bit_rle_decoder <t_word>::read_next_word()
	{
		t_word word{};
		if (!m_stream->read(reinterpret_cast <stream_type::char_type *>(&word), READ_SIZE))
		{
			m_have_next = false;
			return false;
		}
			
		m_next_word = boost::endian::big_to_native(word);
		m_have_next = true;
		return true;
	}
	
	
	template <typename t_word>
	bool bit_rle_decoder <t_word>::prepare()
	{
		return read_next_word();
	}
	
	
	template <typename t_word>
	bool bit_rle_decoder <t_word>::read_next_run(bool &value, std::uint64_t &count)
	{
		if (!m_have_next)
			return false;
		
		value = m_next_word >> COUNT_BITS;
		count = 0;
		std::size_t i(0);
		while (i < 64)
		{
			std::uint64_t count_bits(m_next_word & COUNT_MASK);
			count_bits <<= i;
			count |= count_bits;
			
			i += COUNT_BITS;
			
			if (!read_next_word())
				return true;
			
			bool const next_value(m_next_word >> (COUNT_BITS));
			if (next_value != value)
				return true;
		}
		
		std::uint64_t count_bits(m_next_word & COUNT_MASK);
		count_bits <<= i;
		count |= count_bits;

		read_next_word();
		return true;
	}
	
	
	template <typename t_word>
	void bit_rle_encoder <t_word>::write_run(bool const value, std::uint64_t count)
	{
		while (count)
		{
			t_word const word((value << COUNT_BITS) | (count & COUNT_MASK));
			t_word const to_output(boost::endian::native_to_big(word));
			m_stream->write(reinterpret_cast <stream_type::char_type const *>(&to_output), WRITE_SIZE);
			count >>= COUNT_BITS;
		}
	}
}

#endif

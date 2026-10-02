/*
 * Copyright (c) 2026 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <algorithm>
#include <cstdint>
#include <libbio/assert.hh>
#include <libbio/bit_reading_stream.hh>


namespace libbio {

	void bit_reading_stream::update(std::uint8_t const buffer_start_pos)
	{
		// Update the value in m_buffer using bitwise OR starting from buffer_start_pos.
		// At most 64 - buffer_start_pos bits will be read.
		// The caller is responsible for clearing the buffer starting from buffer_start_pos.

		// Sanity checks.
		libbio_assert_lt(buffer_start_pos, 64U);

		libbio_assert_lte(m_read_pos, m_read_end);
		if (m_read_pos == m_read_end)
			return;

		// Read from the first word.
		position_type offset{};
		position_type word_idx{m_read_pos / 8U};

		{
			auto word{m_source[word_idx]};
			offset = m_read_pos % 64U;
			word >>= offset;
			word <<= buffer_start_pos;
			m_buffer |= word;

			auto const read_amt{std::min(64U - std::max(offset, position_type{buffer_start_pos}), bits_remaining_in_buffer())};
			m_bits_in_buffer += read_amt;
			m_read_pos += read_amt;
		}

		// Check if we need to read additional bits from the next word.
		if (auto const br{bits_remaining_in_buffer()}; buffer_start_pos < offset && br)
		{
			++word_idx;
			auto word{m_source[word_idx]};
			word >>= m_bits_in_buffer;
			m_buffer |= word;

			auto const read_amt{std::min(br, offset - buffer_start_pos)};
			m_bits_in_buffer += read_amt;
			m_read_pos += read_amt;
		}

		{
			// Clear the unused bits of the buffer.
			// Since m_read_pos was less than m_read_end, we have read some bits.
			auto const shift_amt{64U - m_bits_in_buffer};
			auto const mask{UINT64_C(0xFFFF'FFFF'FFFF'FFFF) >> shift_amt};
			m_buffer &= mask;
		}
	}


	void bit_reading_stream::prepare(position_type bit_start, position_type bit_end)
	{
		m_read_pos = bit_start;
		m_read_end = bit_end;
		m_bits_in_buffer = 0;
		m_buffer = 0;
		update();
	}


	void bit_reading_stream::operator>>=(std::uint64_t amt)
	{
		if (64U <= amt)
		{
			// Sanity check.
			auto const br{bits_remaining()};
			if (br < amt) [[unlikely]]
			{
				m_read_pos = m_read_end;
				return;
			}

			// We checked that 64 ≤ amt.
			amt -= m_bits_in_buffer;
			m_read_pos += amt;
			m_buffer = 0;
			update();
			return;
		}

		if (amt < m_bits_in_buffer)
		{
			m_bits_in_buffer -= amt;
			m_buffer >>= amt;
			update(m_bits_in_buffer);
			return;
		}

		// m_bits_in_buffer ≤ amt < 64
		// but we haven’t checked for the number of bits remaining.
		amt -= m_bits_in_buffer;
		m_buffer = 0;
		m_read_pos += std::min(amt, m_read_end - m_read_pos);
		update();
	}
}

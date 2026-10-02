/*
 * Copyright (c) 2026 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <cstdint>
#include <libbio/bit_writing_stream.hh>


namespace libbio {

	bool bit_writing_stream::write_bits(value_type const word, std::uint8_t bit_count)
	{
		// The caller is responsible for setting bits of word starting from bit_count to zero.

		// Sanity check.
		if (m_destination.size() < (m_write_pos + bit_count + 63U) / 64U) return false;

		// Handle the lower bits.
		auto word_idx{m_write_pos / 64U};
		auto const offset{m_write_pos % 64U};
		auto const lower{word << offset};
		m_destination[word_idx] |= lower;
		auto const bits_written{64U - offset};

		// Check if we are done.
		if (bit_count <= bits_written)
		{
			m_write_pos += bit_count;
			return true;
		}

		// Continue.
		++word_idx;
		auto const higher{word >> bits_written};
		m_destination[word_idx] |= higher;
		m_write_pos += bit_count;
		return true;
	}


	bool bit_writing_stream::write_zeros(position_type count)
	{
		auto const write_pos{m_write_pos + count};
		if (m_destination.size() < (write_pos + 63U) / 64U) return false;

		m_write_pos = write_pos;
		return true;
	}
}

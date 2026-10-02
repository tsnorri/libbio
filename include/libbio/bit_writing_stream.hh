/*
 * Copyright (c) 2026 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_BIT_WRITING_STREAM_HH
#define LIBBIO_BIT_WRITING_STREAM_HH

#include <algorithm>
#include <cstdint>
#include <libbio/assert.hh>
#include <span>
#include <vector>


namespace libbio::bit_writing_streams {

	class span_target
	{
	public:
		typedef std::uint64_t position_type;
		typedef std::uint64_t value_type;
		typedef std::span <value_type> target_type;

	private:
		target_type m_values;

	public:
		[[nodiscard]] target_type values() const { return m_values; }
		[[nodiscard]] bool prepare(position_type pos) const { return (pos + 63U) / 64U <= m_values.size(); }
		[[nodiscard]] value_type &operator[](position_type index) { return m_values[index]; }
		void clear() { std::fill(m_values.begin(), m_values.end(), 0); }
	};


	struct vector_target
	{
	public:
		typedef std::uint64_t position_type;
		typedef std::uint64_t value_type;
		typedef std::vector <value_type> target_type;

	private:
		target_type m_values;

	public:
		[[nodiscard]] target_type &values() { return m_values; }
		[[nodiscard]] bool prepare(position_type pos) { m_values.resize(pos, 0); return true; } // FIXME: catch std::bad_alloc?
		[[nodiscard]] value_type &operator[](position_type index) { return m_values[index]; }
		void clear() { m_values.clear(); }
	};
}


namespace libbio {

	template <typename t_target>
	class bit_writing_stream
	{
	public:
		typedef t_target target_type;
		typedef typename target_type::value_type value_type;
		typedef typename target_type::position_type position_type;

	private:
		target_type m_target;
		position_type m_write_pos{};

	public:
		[[nodiscard]] position_type current_position() const { return m_write_pos; }
		[[nodiscard]] target_type &target() { return m_target; }
		[[nodiscard]] bool write_bits(value_type word, std::uint8_t bit_count);
		[[nodiscard]] bool write_zeros(position_type count);
		void clear() { m_target.clear(); m_write_pos = 0; }
	};


	template <typename t_target>
	bool bit_writing_stream <t_target>::write_bits(value_type const word, std::uint8_t bit_count)
	{
		// The caller is responsible for setting bits of word starting from bit_count to zero.

		// Sanity check.
		if (!m_target.prepare(m_write_pos + bit_count)) return false;

		// Handle the lower bits.
		auto word_idx{m_write_pos / 64U};
		auto const offset{m_write_pos % 64U};
		auto const lower{word << offset};
		m_target[word_idx] |= lower;
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
		m_target[word_idx] |= higher;
		m_write_pos += bit_count;
		return true;
	}


	template <typename t_target>
	bool bit_writing_stream <t_target>::write_zeros(position_type count)
	{
		auto const write_pos{m_write_pos + count};
		if (!m_target.prepare(write_pos)) return false;

		m_write_pos = write_pos;
		return true;
	}
}

#endif

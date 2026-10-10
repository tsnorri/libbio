/*
 * Copyright (c) 2026 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_BIT_READING_STREAM_HH
#define LIBBIO_BIT_READING_STREAM_HH

#include "libbio/bits.hh"
#include <climits>
#include <cstdint>
#include <cstddef>
#include <libbio/assert.hh>
#include <span>


namespace libbio {

	class bit_reading_stream
	{
	public:
		typedef std::span <std::uint64_t const> source_type;
		typedef std::uint64_t position_type;
		typedef std::uint64_t value_type;
		typedef std::uint8_t bit_count_type;
		constexpr static inline bit_count_type value_bits{sizeof(value_type) * CHAR_BIT};

		struct invalid_tag {};

	private:
		source_type m_source{};
		position_type m_read_pos{};
		position_type m_read_end{};

	public:
		bit_reading_stream() = default;

		explicit bit_reading_stream(source_type source):
			m_source{source}
		{
		}

		// For e.g. an old-style end iterator.
		bit_reading_stream(position_type position, invalid_tag):
			m_read_pos{position},
			m_read_end{position}
		{
		}

		bit_reading_stream(
			source_type source,
			position_type read_pos,
			position_type read_end
		):
			m_source{source},
			m_read_pos{read_pos},
			m_read_end{read_end}
		{
		}

		source_type &source() { return m_source; }

		position_type current_position() const { return m_read_pos; }
		position_type end_position() const { return m_read_end; }
		void set_current_position(position_type pos) { m_read_pos = pos; }
		void set_end_position(position_type pos) { m_read_end = pos; }

		[[nodiscard]] position_type bits_remaining() const { return m_read_end - m_read_pos; }
		[[nodiscard]] operator bool() const { return bits_remaining(); }
		[[nodiscard]] value_type read_bits(bit_count_type const amount) const;
		void seek_forward(position_type amt) { libbio_assert_lte(m_read_pos + amt, m_read_end); m_read_pos += amt; }
		void operator>>=(position_type amt) { seek_forward(amt); }

		template <std::size_t t_size>
		void copy_to(std::span <value_type, t_size> span) const;
	};


	inline auto bit_reading_stream::read_bits(bit_count_type const amount) const -> value_type
	{
		if (0 == amount) return {};

		libbio_assert_lte(amount, value_bits);

		// Read from the first word.
		auto const word_index{m_read_pos / value_bits};
		auto const offset{m_read_pos % value_bits};
		value_type retval{m_source[word_index] >> offset};

		// Read from the next word.
		// It may be that next_word_index == word_index but we
		// substitute the branch with a bitwise AND.
		auto const next_word_index{(m_read_pos + amount - 1) / value_bits};
		auto const next_shift{(value_bits - offset) % value_bits};
		retval |= m_source[next_word_index] << next_shift;

		// Apply a mask.
		// Since we can read value_bits bits, we need to subtract one
		// but we already checked that 0 < amount.
		value_type const mask{~((~(value_type{1})) << (amount -  1))};
		retval &= mask;

		return retval;
	}


	template <std::size_t t_size>
	void bit_reading_stream::copy_to(std::span <value_type, t_size> dst) const
	{
		// Copy at most as many words as can fit in the given span.
		// The copy will be made in such a way that the words are first copied
		// to the buffer and then shifted by the bit offset indicated by the
		// current reading position.

		auto const word_begin{m_read_pos % value_bits};
		auto const read_end_{(word_begin + dst.size()) * value_bits};
		auto const read_end{std::min(m_read_end, read_end_)};
		auto const word_count{(read_end - m_read_pos + value_bits - 1) / value_bits};

		std::copy_n(m_source.begin() + word_begin, word_count, dst.begin());
		bits::shift_span_right(dst, m_read_pos % value_bits);
	}
}

#endif

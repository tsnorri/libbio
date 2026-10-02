/*
 * Copyright (c) 2026 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_BIT_READING_STREAM_HH
#define LIBBIO_BIT_READING_STREAM_HH

#include <cstdint>
#include <libbio/assert.hh>
#include <span>


namespace libbio {

	class bit_reading_stream
	{
	public:
		typedef std::span <std::uint64_t const> source_type;
		typedef std::uint64_t position_type;
		typedef std::uint64_t value_type;

		struct invalid_tag {};

	private:
		source_type m_source{};
		std::uint64_t m_buffer{};
		position_type m_read_pos{};
		position_type m_read_end{};
		std::uint8_t m_bits_in_buffer{};

	private:
		void update(std::uint8_t buffer_start = 0);

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
			update();
		}

		source_type &source() { return m_source; }

		position_type current_position() const { return m_read_pos; }
		position_type end_position() const { return m_read_end; }

		void assign(source_type source) { m_source = source; };
		void prepare(position_type bit_start, position_type bit_end);

		[[nodiscard]] position_type bits_remaining() const { return m_read_end - m_read_pos + m_bits_in_buffer; }
		[[nodiscard]] position_type bits_remaining_in_buffer() const { return m_read_end - m_read_pos; }
		[[nodiscard]] operator bool() const { return bits_remaining(); }
		[[nodiscard]] inline value_type value() const;
		void operator>>=(position_type amt); // Right shift.
	};


	inline std::uint64_t bit_reading_stream::value() const
	{
		// Precondition: m_bits_in_buffer is non-zero.
		libbio_assert_lt(0, m_bits_in_buffer);
		auto const shift_amt{64U - m_bits_in_buffer};
		auto const mask{UINT64_C(0xFFFF'FFFF'FFFF'FFFF) >> shift_amt};
		auto const retval{m_buffer & mask};
		return retval;
	}
}

#endif

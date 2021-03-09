/*
 * Copyright (c) 2021 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_BUFFERED_WRITER_BASE_HH
#define LIBBIO_BUFFERED_WRITER_BASE_HH

#include <cstddef>
#include <string_view>
#include <vector>


namespace libbio {
	
	struct character_count
	{
		std::size_t	count{};
		char		character{};
		
		character_count() = default;
		
		character_count(char character_, std::size_t count_):
			count(count_),
			character(character_)
		{
		}
	};
	
	
	class buffered_writer_base
	{
	public:
		typedef std::vector <char>	buffer_type;
		
	protected:
		buffer_type	m_buffer;
		std::size_t	m_position{};
		std::size_t	m_output_position{};
		
	public:
		buffered_writer_base() = default;
		
		explicit buffered_writer_base(std::size_t buffer_size):
			m_buffer(buffer_size, 0)
		{
		}
		
		virtual ~buffered_writer_base() {}
		
		// FIXME: consider making the following templated non-members to be able to return an object of the same type. (On the other hand, only the call to flush() will be virtual.)
		inline buffered_writer_base &operator<<(char const c);
		buffered_writer_base &operator<<(std::string_view const sv);
		buffered_writer_base &operator<<(character_count c);
		
		std::size_t tellg() const { return m_output_position; }
		
		virtual void flush() = 0;
	};
	
	
	// Write one copy of c.
	buffered_writer_base &buffered_writer_base::operator<<(char const c)
	{
		m_buffer[m_position] = c;
		++m_position;
		if (m_position == m_buffer.size())
			flush();
		
		return *this;
	}
}

#endif
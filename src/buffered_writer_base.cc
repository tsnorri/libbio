/*
 * Copyright (c) 2021-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <algorithm>
#include <libbio/buffered_writer/buffered_writer_base.hh>
#include <string_view>


namespace libbio {

	// Write the contents of sv.
	buffered_writer_base &buffered_writer_base::operator<<(std::string_view const sv)
	{
		auto it(sv.begin());
		auto remaining(sv.size());

		while (remaining)
		{
			auto const chunk_size(std::min(m_buffer.size() - m_position, remaining));
			std::copy_n(it, chunk_size, m_buffer.begin() + m_position);

			m_position += chunk_size;
			if (m_position == m_buffer.size())
				flush();

			it += chunk_size;
			remaining -= chunk_size;
		}

		return *this;
	}


	// Write cc.count copies of cc.character.
	buffered_writer_base &buffered_writer_base::operator<<(character_count cc)
	{
		while (cc.count)
		{
			auto const chunk_size(std::min(m_buffer.size() - m_position, cc.count));
			std::fill_n(m_buffer.begin() + m_position, chunk_size, cc.character);

			m_position += chunk_size;
			if (m_position == m_buffer.size())
				flush();

			cc.count -= chunk_size;
		}

		return *this;
	}
}

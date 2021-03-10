/*
 * Copyright (c) 2021 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <cstring>
#include <libbio/buffered_writer/file_handle_buffered_writer.hh>
#include <unistd.h>


namespace libbio {
	
	void file_handle_buffered_writer::flush()
	{
		auto const byte_count(this->m_position);
		if (byte_count)
		{
			auto const res(write(this->m_fd, this->m_buffer.data(), byte_count));
			if (res != this->m_position)
				throw std::runtime_error(std::strerror(errno));
			this->m_position = 0;
			this->m_output_position += byte_count;
		}
	}
}

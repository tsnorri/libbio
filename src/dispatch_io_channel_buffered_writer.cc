/*
 * Copyright (c) 2021 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <libbio/buffered_writer/dispatch_io_channel_buffered_writer.hh>
#include <mutex>
#include <unistd.h>


namespace libbio {
	
	void dispatch_io_channel_buffered_writer::flush()
	{
		auto const byte_count(this->m_position);
		if (byte_count)
		{
			// FIXME: make sure that nothing below throws, since the destructor of the dispatch data below is responsible for unlocking the semaphore.
			m_writing_lock.lock();
		
			// Swap the buffers.
			using std::swap;
			swap(m_writing_buffer, this->m_buffer);
			
			// Mark the original buffer empty.
			this->m_position = 0;
			
			// Create dispatch data and schedule a writing operation.
			auto concurrent_queue(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0));
			
			auto const *buffer(m_writing_buffer.data());
			dispatch_ptr dispatch_data(dispatch_data_create(buffer, byte_count, concurrent_queue, ^{ this->m_writing_lock.unlock(); }));
			
			dispatch_io_write(*m_io_channel, this->m_output_position, *dispatch_data, concurrent_queue, ^(bool done, dispatch_data_t data, int error){
				if (error)
				{
					dispatch_async(*this->m_reporting_queue, ^{
						throw std::runtime_error(std::strerror(error));
					});
				}
			});
			
			// Update the output position.
			this->m_output_position += byte_count;
		}
	}
	
	
	void dispatch_io_channel_buffered_writer::close()
	{
		flush();
		
		// The lock is released only after the last writing operation has concluded.
		// Close the file after that.
		std::lock_guard const lock(m_writing_lock);
		dispatch_io_close(*m_io_channel, 0);
	}
}

/*
 * Copyright (c) 2021 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_DISPATCH_IO_CHANNEL_BUFFERED_WRITER_HH
#define LIBBIO_DISPATCH_IO_CHANNEL_BUFFERED_WRITER_HH

#include <libbio/buffered_writer/buffered_writer_base.hh>
#include <libbio/dispatch.hh>


namespace libbio {
	
	class dispatch_io_channel_buffered_writer final : public buffered_writer_base
	{
	protected:
		dispatch_ptr <dispatch_io_t>	m_io_channel;
		dispatch_ptr <dispatch_queue_t>	m_reporting_queue;
		dispatch_semaphore_lock			m_writing_lock;
		buffer_type						m_writing_buffer;
		
	public:
		dispatch_io_channel_buffered_writer() = default;
		
		// FIXME: error handling could be done in some other way than throwing.
		dispatch_io_channel_buffered_writer(dispatch_fd_t fd, std::size_t buffer_size, dispatch_queue_t reporting_queue):
			buffered_writer_base(buffer_size),
			m_io_channel(dispatch_io_create(DISPATCH_IO_RANDOM, fd, reporting_queue, ^(int error){ if (error) throw std::runtime_error(strerror(error)); })),
			m_reporting_queue(reporting_queue, true),
			m_writing_lock(1),
			m_writing_buffer(buffer_size, 0)
		{
			if (!m_io_channel)
				throw std::runtime_error("Unable to create IO channel");
		}
		
		// FIXME: same as above.
		dispatch_io_channel_buffered_writer(char const *path, int oflag, mode_t mode, std::size_t buffer_size, dispatch_queue_t reporting_queue):
			buffered_writer_base(buffer_size),
			m_io_channel(dispatch_io_create_with_path(DISPATCH_IO_RANDOM, path, oflag, mode, reporting_queue, ^(int error){ if (error) throw std::runtime_error(strerror(error)); })),
			m_reporting_queue(reporting_queue, true),
			m_writing_lock(1),
			m_writing_buffer(buffer_size, 0)
		{
			if (!m_io_channel)
				throw std::runtime_error("Unable to create IO channel");
		}
			
		~dispatch_io_channel_buffered_writer() { close(); }
		
		dispatch_io_channel_buffered_writer(dispatch_io_channel_buffered_writer &&) = default;
		dispatch_io_channel_buffered_writer &operator=(dispatch_io_channel_buffered_writer &&) & = default;
		
		// Copying not allowed.
		dispatch_io_channel_buffered_writer(dispatch_io_channel_buffered_writer const &) = delete;
		dispatch_io_channel_buffered_writer &operator=(dispatch_io_channel_buffered_writer const &) = delete;
		
		void close();
		void flush() override;
	};
}
#endif

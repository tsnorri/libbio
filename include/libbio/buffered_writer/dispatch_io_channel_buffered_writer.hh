/*
 * Copyright (c) 2021 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_DISPATCH_IO_CHANNEL_BUFFERED_WRITER_HH
#define LIBBIO_DISPATCH_IO_CHANNEL_BUFFERED_WRITER_HH

#include <libbio/buffered_writer/buffered_writer_base.hh>
#include <libbio/dispatch.hh>


namespace libbio {
	
	enum class dispatch_io_channel_flags : std::uint8_t
	{
		NONE					= 0x0,
		HAS_RANDOM_ACCESS		= 0x1,
		OWNS_FILE_DESCRIPTOR	= 0x2
	};
	
	
	constexpr inline dispatch_io_channel_flags
	operator|(dispatch_io_channel_flags const ll, dispatch_io_channel_flags const rr)
	{
		return static_cast <dispatch_io_channel_flags>(libbio::to_underlying(ll) | libbio::to_underlying(rr));
	}
	
	
	constexpr inline dispatch_io_channel_flags
	operator&(dispatch_io_channel_flags const ll, dispatch_io_channel_flags const rr)
	{
		return static_cast <dispatch_io_channel_flags>(libbio::to_underlying(ll) & libbio::to_underlying(rr));
	}
	
	
	class dispatch_io_channel_buffered_writer final : public buffered_writer_base
	{
	protected:
		dispatch_ptr <dispatch_io_t>	m_io_channel;
		dispatch_ptr <dispatch_queue_t>	m_reporting_queue;
		dispatch_semaphore_lock			m_writing_lock;
		buffer_type						m_writing_buffer;
		bool							m_owns_file_descriptor{};
		
	public:
		dispatch_io_channel_buffered_writer() = default;
		
		// FIXME: error handling could be done in some other way than throwing.
		dispatch_io_channel_buffered_writer(dispatch_fd_t fd, std::size_t buffer_size, dispatch_queue_t reporting_queue, dispatch_io_channel_flags const ff = dispatch_io_channel_flags::HAS_RANDOM_ACCESS):
			buffered_writer_base(buffer_size),
			m_io_channel(
				dispatch_io_create( // According to the man page, does not close() the file descriptor.
					to_underlying(ff & dispatch_io_channel_flags::HAS_RANDOM_ACCESS) ? DISPATCH_IO_RANDOM : DISPATCH_IO_STREAM,
					fd,
					reporting_queue,
					^(int error){ if (error) throw std::runtime_error(strerror(error)); }
				)
			),
			m_reporting_queue(reporting_queue, true),
			m_writing_lock(1),
			m_writing_buffer(buffer_size, 0),
			m_owns_file_descriptor(to_underlying(ff & dispatch_io_channel_flags::OWNS_FILE_DESCRIPTOR) ? true : false)
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
			
		~dispatch_io_channel_buffered_writer() { if (m_io_channel) close(); }
		
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

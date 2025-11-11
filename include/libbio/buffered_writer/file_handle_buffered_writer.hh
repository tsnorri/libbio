/*
 * Copyright (c) 2021-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_FILE_HANDLE_BUFFERED_WRITER_HH
#define LIBBIO_FILE_HANDLE_BUFFERED_WRITER_HH

#include <cstddef>
#include <libbio/buffered_writer/buffered_writer_base.hh>
#include <libbio/file_handle.hh>


namespace libbio {

	class file_handle_buffered_writer final : public file_handle_, public buffered_writer_base
	{
	public:
		file_handle_buffered_writer() = default;

		file_handle_buffered_writer(file_descriptor_type fd, std::size_t buffer_size):
			file_handle_(fd),
			buffered_writer_base(buffer_size)
		{
		}

		virtual ~file_handle_buffered_writer() { flush(); }

		void flush() override;

		file_handle_buffered_writer(file_handle_buffered_writer &&) = default;
		file_handle_buffered_writer &operator=(file_handle_buffered_writer &&) & = default;

		// Copying not allowed.
		file_handle_buffered_writer(file_handle_buffered_writer const &) = delete;
		file_handle_buffered_writer &operator=(file_handle_buffered_writer const &) = delete;
	};
}
#endif

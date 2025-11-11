/*
 * Copyright (c) 2025 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_SEQUENCE_READER_HH
#define LIBBIO_SEQUENCE_READER_HH

#include <cstddef>
#include <cstdint>
#include <libbio/file_handle.hh>


namespace libbio {

	struct sequence_reader
	{
		typedef reading_handle	handle_type;

		enum class parsing_status {
			success,
			failure,
			cancelled
		};

		virtual ~sequence_reader() {}

		virtual parsing_status parse(handle_type &handle, std::size_t blocksize) = 0;
		parsing_status parse(handle_type &handle) { return parse(handle, handle.io_op_blocksize()); }

		virtual void prepare() = 0;
		virtual parsing_status parse_(handle_type &handle, std::size_t blocksize) = 0;
		parsing_status parse_(handle_type &handle) { return parse_(handle, handle.io_op_blocksize()); }

		virtual std::uint64_t line_number() const = 0;
	};
}

#endif

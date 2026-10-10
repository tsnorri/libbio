/*
 * Copyright (c) 2026 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_TESTS_SPAN_BIT_STREAMS_HH
#define LIBBIO_TESTS_SPAN_BIT_STREAMS_HH

#include <array>
#include <cstdint>
#include <cstddef>
#include <libbio/bit_reading_stream.hh>
#include <libbio/bit_writing_stream.hh>
#include <span>

namespace lb		= libbio;


namespace libbio::tests {

	struct span_writing_stream
	{
		constexpr static inline std::size_t size{4};

		typedef std::uint64_t value_type;
		typedef std::array <value_type, size> buffer_type;
		typedef std::span <value_type, size> span_type;
		typedef lb::bit_writing_streams::span_target <span_type::extent> target_type;

		buffer_type buffer{};
		lb::bit_writing_stream <target_type> stream;

		span_writing_stream():
			stream{
				target_type{
					span_type{buffer}
				}
			}
		{
		}
	};


	struct span_reading_stream
	{
		lb::bit_reading_stream stream;

		explicit span_reading_stream(span_writing_stream const writing_stream):
			stream{
				writing_stream.stream.target().values()
			}
		{
		}
	};
}

#endif

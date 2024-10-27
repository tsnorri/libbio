/*
 * Copyright (c) 2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_BAM_UNORDERED_STREAMING_READER_HH
#define LIBBIO_BAM_UNORDERED_STREAMING_READER_HH

#include <atomic>							// std::atomic_flag
#include <cstddef>
#include <libbio/bam/header.hh>
#include <libbio/bgzf/streaming_reader.hh>
#include <libbio/sam/header.hh>
#include <libbio/sam/record.hh>


namespace libbio::bam {

	class unordered_streaming_reader;


	struct unordered_streaming_reader_delegate
	{
		virtual ~unordered_streaming_reader_delegate() {}
		virtual void streaming_reader_did_parse_header(unordered_streaming_reader &reader, header &&hh, sam::header &&hh_) = 0;
		virtual void streaming_reader_did_parse_record(unordered_streaming_reader &reader, sam::record &record) = 0;
	};


	class unordered_streaming_reader : public bgzf::streaming_reader_delegate
	{
		typedef bgzf::streaming_reader::output_buffer_type	bgzf_buffer_type;
		typedef unordered_streaming_reader_delegate			delegate_type;

		delegate_type										*m_delegate{};
		std::atomic_flag									m_seen_header{};

	public:
		explicit unordered_streaming_reader(delegate_type &delegate):
			m_delegate(&delegate)
		{
		}

		void streaming_reader_did_decompress_block(
			bgzf::streaming_reader &reader,
			std::size_t block_index,
			bgzf_buffer_type &buffer
		) override;
	};
}

#endif

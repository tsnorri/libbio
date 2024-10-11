/*
 * Copyright (c) 2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <libbio/bam/header_parser.hh>
#include <libbio/bam/record_parser.hh>
#include <libbio/bam/unordered_streaming_reader.hh>


namespace libbio::bam {
	
	void unordered_streaming_reader::streaming_reader_did_decompress_block(
		bgzf::streaming_reader &reader,
		std::size_t block_index,
		bgzf_buffer_type &buffer
	)
	{
		binary_parsing::range range{buffer.data(), buffer.size()};
		
		// Block until the header has been delivered to the delegate.
		if (block_index)
			m_seen_header.wait(false, std::memory_order_acquire);
		else
		{
			header hh;
			sam::header hh_;
			
			detail::read_header(range, hh, hh_);
			m_delegate->streaming_reader_did_parse_header(*this, std::move(hh), std::move(hh_));
			m_seen_header.test_and_set(std::memory_order_release);
			m_seen_header.notify_all();
		}
		
		sam::record record;
		while (range)
		{
			record_parser parser(range, record);
			parser.parse();
			m_delegate->streaming_reader_did_parse_record(*this, record);
		}
		
		reader.return_output_buffer(buffer);
		// buffer is now invalid.
	}
}

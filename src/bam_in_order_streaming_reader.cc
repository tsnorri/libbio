/*
 * Copyright (c) 2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <libbio/bam/header_parser.hh>
#include <libbio/bam/in_order_streaming_reader.hh>
#include <libbio/bam/record_parser.hh>


namespace libbio::bam {
	
	void in_order_streaming_reader::prepare_for_next_block_and_return_record_buffer(record_buffer &&buffer)
	{
		++m_next_block_index;
	
		{
			std::lock_guard const lock(m_buffer_mutex);
			m_expected_block_index = m_next_block_index;
			m_record_buffers.emplace_back(std::move(buffer));
		}

		m_next_block_reading_cv.notify_one();
		m_arbitrary_block_reading_cv.notify_one();
	}
	
	
	void in_order_streaming_reader::assign_record_buffer_or_wait(record_block &block)
	{
		std::unique_lock lock(m_buffer_mutex);
		while (true)
		{
			if (1 < m_record_buffers.size() || block.index == m_expected_block_index)
			{
				libbio_assert(!m_record_buffers.empty());
				
				using std::swap;
				swap(block.records, m_record_buffers.back());
				m_record_buffers.pop_back();
				break;
			}
			
			if (block.index == 1 + m_expected_block_index)
				m_next_block_reading_cv.wait(lock);
			else
				m_arbitrary_block_reading_cv.wait(lock);
		}
	}
	
	
	void in_order_streaming_reader::streaming_reader_did_decompress_block(
		bgzf::streaming_reader &reader,
		std::size_t block_index,
		bgzf_buffer_type &buffer
	)
	{
		record_block block(block_index);
		binary_parsing::range range{buffer.data(), buffer.size()};
		
		assign_record_buffer_or_wait(block);
		
		if (0 == block_index)
		{
			header hh;
			sam::header hh_;
			
			detail::read_header(range, hh, hh_);
			
			// Should be enough for streaming_reader_did_parse_header() to get called first.
			m_queue->group_async(*m_group, [hh = std::move(hh), hh_ = std::move(hh_), this] mutable {
				m_delegate->streaming_reader_did_parse_header(*this, std::move(hh), std::move(hh_));
			});
		}
		
		block.records.clear();
		while (range)
		{
			auto &record(block.records.next_record());
			record_parser parser(range, record);
			parser.parse();
		}
		
		reader.return_output_buffer(buffer);
		// buffer is now invalid.
		
		// Pass the parsed records to the delegate.
		m_queue->group_async(*m_group, [block = std::move(block), this] mutable {
			if (m_next_block_index == block.index)
			{
				m_delegate->streaming_reader_did_parse_records(*this, block.records);
				prepare_for_next_block_and_return_record_buffer(std::move(block.records));
			}
			else
			{
				m_pending_blocks.emplace_back(std::move(block));
				std::push_heap(m_pending_blocks.begin(), m_pending_blocks.end(), std::greater <>{});
			}
			
			while (!m_pending_blocks.empty() && m_next_block_index == m_pending_blocks.front().index)
			{
				std::pop_heap(m_pending_blocks.begin(), m_pending_blocks.end(), std::greater <>{});
				auto block_(std::move(m_pending_blocks.back()));
				m_pending_blocks.pop_back();
				m_delegate->streaming_reader_did_parse_records(*this, block_.records);
				prepare_for_next_block_and_return_record_buffer(std::move(block_.records));
			}
		});
	}
}

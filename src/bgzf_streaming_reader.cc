/*
 * Copyright (c) 2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <libbio/bgzf/parser.hh>
#include <libbio/bgzf/streaming_reader.hh>


namespace libbio::bgzf::detail {
	
	void streaming_reader_decompression_task::run()
	{
		auto &dst(reader->m_buffer_queue.pop());
		dst.resize(block.isize, std::byte{});
		auto const res(decompressor.decompress(
			std::span{block.compressed_data, block.compressed_data_size},
			std::span{dst.data(), dst.size()}
		));
		
		if (res.size() != block.isize)
			throw std::runtime_error("Unexpected number of bytes decompressed from a BGZF block");
		
		libbio_assert(reader);
		reader->decompression_task_did_finish(*this, dst);
	}
}


namespace libbio::bgzf {
	
	void streaming_reader::decompression_task_did_finish(decompression_task &task, output_buffer_type &buffer)
	{
		{
			std::lock_guard const lock{m_released_offsets_mutex};
			m_released_offsets.push_back(task.block.compressed_data);
		}
		
		auto const block_index(task.block_index);
		m_task_queue.push(task);
		// Task no longer valid.
		m_delegate->streaming_reader_did_decompress_block(*this, block_index, buffer);
	}
	
	
	void streaming_reader::return_output_buffer(output_buffer_type &buffer)
	{
		m_buffer_queue.push(buffer);
	}
	
	
	void streaming_reader::run(dispatch::queue &dispatch_queue)
	{
		// To process the file, we first fill m_input_buffer with its contents.
		// We then process the buffer contents until there are less than block_size bytes left or, in
		// case EOF has been reached, until the end.
		//
		// Processing is done by reading the consecutive BGZF blocks and starting a decompression task
		// possibly mediated by a semaphore. When starting a block, we call add_buffer_offset. When a
		// task is done, we remove said offset, adjust the beginning of the circular buffer to the
		// next smallest offset and fill the buffer again.
		
		m_input_buffer.clear();
		m_active_offsets.clear();
		m_released_offsets.clear();
		m_offset_buffer.clear();
		std::size_t block_index{};
		
		circular_buffer::const_range reading_range{};
		
		auto const decompress_block([this, &block_index, &dispatch_queue](block &bb){
			auto &task(m_task_queue.pop()); // Blocks when no more tasks are available.
			task.block = bb;
			task.block_index = block_index++;
			dispatch_queue.group_async(*m_group, &task);
		});
		
		while (true)
		{
			// Update the buffer.
			std::size_t bytes_read{};
			
			{
				auto writing_range(m_input_buffer.writing_range());
				libbio_always_assert_lt(0, writing_range.size());
				bytes_read = m_handle->read(writing_range.size(), writing_range.it);
				if (0 == bytes_read)
					break;
				
				m_input_buffer.add_to_occupied(bytes_read);
			}
			
			reading_range = m_input_buffer.reading_range();
			binary_parsing::range reading_range_(reading_range);
			
			// Read until at most block_size bytes left.
			while (block_size < reading_range.size())
			{
				// Parse a BGZF block.
				block bb;
				parser pp(reading_range_, bb);
				pp.parse();
				reading_range.it = reading_range_.it;
				
				// Store the offset of the compressed stream.
				m_active_offsets.push_back(bb.compressed_data);
				
				// Start a decompression task.
				decompress_block(bb);
			}
			
			// Update the offsets.
			m_offset_buffer.clear();
			
			{
				std::lock_guard const lock{m_released_offsets_mutex};
				std::sort(m_released_offsets.begin(), m_released_offsets.end());
				libbio_assert(std::is_sorted(m_active_offsets.begin(), m_active_offsets.end()));
				std::set_difference(
					m_active_offsets.begin(), m_active_offsets.end(),
					m_released_offsets.begin(), m_released_offsets.end(),
					std::back_inserter(m_offset_buffer)
				);
				m_released_offsets.clear();
			}
			
			{
				using std::swap;
				swap(m_active_offsets, m_offset_buffer);
			}
			
			// Update the position.
			if (m_active_offsets.empty())
				m_input_buffer.clear();
			else
				m_input_buffer.set_begin(m_active_offsets.front());
		}
		
		// EOF found. Read until the end of the input.
		{
			binary_parsing::range reading_range_(reading_range);
			while (reading_range_)
			{
				// Parse a BGZF block.
				block bb;
				parser pp(reading_range_, bb);
				pp.parse();
				reading_range.it = reading_range_.it;
			
				// Start a decompression task.
				decompress_block(bb);
			}
		}
	}
}

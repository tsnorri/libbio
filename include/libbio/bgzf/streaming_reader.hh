/*
 * Copyright (c) 2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_BGZF_STREAMING_READER_HH
#define LIBBIO_BGZF_STREAMING_READER_HH

#include <libbio/bgzf/block.hh>
#include <libbio/bgzf/deflate_decompressor.hh>
#include <libbio/bounded_mpmc_queue.hh>
#include <libbio/circular_buffer.hh>
#include <libbio/dispatch.hh>
#include <libbio/file_handle.hh>
#include <mutex>
#include <semaphore>
#include <thread>								// std::thread::hardware_concurrency()
#include <vector>


namespace libbio::bgzf {

	class streaming_reader;
	typedef std::vector <std::byte> streaming_reader_output_buffer_type;
	
	
	struct streaming_reader_delegate
	{
		typedef streaming_reader_output_buffer_type output_buffer_type;
		
		virtual ~streaming_reader_delegate() {}
		
		// Will be called from worker threads.
		virtual void streaming_reader_did_decompress_block(
			streaming_reader &reader,
			std::size_t block_index,
			output_buffer_type &buffer
		) = 0;
	};
}


namespace libbio::bgzf::detail {
	
	struct streaming_reader_decompression_task
	{
		typedef streaming_reader_output_buffer_type	output_buffer_type;
		
		deflate_decompressor	decompressor;
		struct block			block;
		streaming_reader		*reader{};
		std::size_t				block_index{};
		
		void prepare() { decompressor.prepare(); }
		void run();
		void operator()() { run(); }
	};
}


namespace libbio::bgzf {
	
	/*
	 * Read a BGZF blocks as a stream but not necessarily in-order (as opposed to random access).
	 *
	 * Idea:
	 * – Maintain a (circular) input buffer the size of which is some multiple of a BGZF block (64KB).
	 * – Read the block header, set the compressed stream pointer in bgzf::block.
	 * – Decompress (in a worker thread).
	 * – Mark the buffer unused when the decompressed data is ready. If this was the (linearly) leftmost
	 *   block, make space available in the buffer so that more data may be read from disk.
	 */
	class streaming_reader
	{
		typedef detail::streaming_reader_decompression_task	decompression_task;
		
		friend decompression_task;
		
	public:
		typedef streaming_reader_output_buffer_type		output_buffer_type;
	
	private:
		typedef std::counting_semaphore <UINT16_MAX>	semaphore_type;
		typedef bounded_mpmc_queue <decompression_task>	task_queue_type; // Actually only MPSC needed.
		typedef bounded_mpmc_queue <output_buffer_type>	buffer_queue_type;
		typedef std::vector <std::uintptr_t>			offset_vector;
		
	public:
		constexpr static std::size_t const block_size{65536};
		
	private:
		circular_buffer									m_input_buffer;
		task_queue_type									m_task_queue;
		buffer_queue_type								m_buffer_queue;
		offset_vector									m_active_offsets;
		offset_vector									m_released_offsets;
		offset_vector									m_offset_buffer;
		semaphore_type									*m_semaphore{};
		file_handle										*m_handle{};
		dispatch::group									*m_group{};
		streaming_reader_delegate						*m_delegate{};
		std::mutex										m_released_offsets_mutex{};
	
	private:
		static std::size_t page_count_for_buffer(std::size_t task_count) { return bits::gte_power_of_2_((task_count * block_size / circular_buffer::page_size()) ?: 1); }
		void decompression_task_did_finish(decompression_task &task, output_buffer_type &decompressed_data);
	
	public:
		streaming_reader(
			file_handle &handle,
			std::size_t const task_count,							// (Likely) need to be less than the maximum number of threads to avoid deadlocks.
			std::size_t const buffer_count,
			dispatch::group &group,
			semaphore_type *semaphore,								// Optional
			streaming_reader_delegate &delegate
		):
			m_input_buffer(2 * page_count_for_buffer(task_count)),	// Make sure that we don’t run out of space while reading.
			m_task_queue(task_count, task_queue_type::start_from_reading{true}),
			m_buffer_queue(buffer_count, buffer_queue_type::start_from_reading{true}),
			m_semaphore(semaphore),
			m_handle(&handle),
			m_group(&group),
			m_delegate(&delegate)
		{
			libbio_assert_lt(0, task_count);
			libbio_assert_lte(2 * block_size, m_input_buffer.size());
			for (auto &task : m_task_queue.values())
			{
				task.reader = this;
				task.prepare();
			}
		}
		
		streaming_reader(
			file_handle &handle,
			std::size_t const task_count,
			dispatch::group &group,
			semaphore_type *semaphore,								// Optional
			streaming_reader_delegate &delegate
		):
			streaming_reader(handle, task_count, 2 * task_count, group, semaphore, delegate)
		{
		}
		
		streaming_reader(
			file_handle &handle,
			dispatch::group &group,
			semaphore_type *semaphore,								// Optional
			streaming_reader_delegate &delegate
		):
			streaming_reader(handle, std::thread::hardware_concurrency() ?: 1, group, semaphore, delegate)
		{
		}
		
		void run(dispatch::queue &queue = dispatch::parallel_queue::shared_queue());
		void return_output_buffer(output_buffer_type &buffer);
	};
}

#endif

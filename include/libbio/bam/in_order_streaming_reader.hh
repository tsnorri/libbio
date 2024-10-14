/*
 * Copyright (c) 2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_BAM_IN_ORDER_STREAMING_READER_HH
#define LIBBIO_BAM_IN_ORDER_STREAMING_READER_HH

#include <condition_variable>
#include <libbio/bam/header.hh>
#include <libbio/bam/record_buffer.hh>
#include <libbio/bgzf/streaming_reader.hh>
#include <libbio/dispatch/group.hh>
#include <libbio/dispatch/queue.hh>
#include <libbio/sam/header.hh>
#include <mutex>
#include <vector>


namespace libbio::bam {
	
	class in_order_streaming_reader;
	
	
	struct in_order_streaming_reader_delegate
	{
		virtual ~in_order_streaming_reader_delegate() {}
		virtual void streaming_reader_did_parse_header(in_order_streaming_reader &reader, header &&hh, sam::header &&hh_) = 0;
		virtual void streaming_reader_did_parse_records(in_order_streaming_reader &reader, record_buffer &records) = 0;
	};
	
	
	class in_order_streaming_reader : public bgzf::streaming_reader_delegate
	{
	public:
		typedef in_order_streaming_reader_delegate			delegate_type;
		
	private:
		typedef bgzf::streaming_reader::output_buffer_type	bgzf_buffer_type;
		
		struct record_block
		{
			std::size_t								index{};
			record_buffer							records;
			
			explicit record_block(std::size_t index_):
				index(index_)
			{
			}
			
			constexpr bool operator<(record_block const &other) const { return index < other.index; }
			constexpr bool operator>(record_block const &other) const { return index > other.index; }
		};
		
		typedef std::vector <record_buffer>			record_buffer_vector;
		typedef std::vector <record_block>			record_block_vector;
		
	private:
		alignas(std::hardware_destructive_interference_size) std::size_t	m_expected_block_index{};
		std::condition_variable												m_arbitrary_block_reading_cv;
		std::condition_variable												m_next_block_reading_cv;
		std::mutex															m_buffer_mutex;					// Protects m_record_buffers and m_expected_block_index.
		record_buffer_vector												m_record_buffers;
		record_block_vector													m_pending_blocks;
		std::size_t															m_next_block_index{};			// Accessed only from m_queue.
		dispatch::serial_queue_base											*m_queue{};
		dispatch::group														*m_group{};
		delegate_type														*m_delegate{};
		
	private:
		void assign_record_buffer_or_wait(record_block &block);
		void prepare_for_next_block_and_return_record_buffer(record_buffer &&buffer);
		
	public:
		in_order_streaming_reader(
			std::size_t buffer_count,
			dispatch::serial_queue_base &queue,
			dispatch::group &group,
			delegate_type &delegate
		):
			m_record_buffers(buffer_count),
			m_queue(&queue),
			m_group(&group),
			m_delegate(&delegate)
		{
		}
		
		in_order_streaming_reader(
			dispatch::serial_queue_base &queue,
			dispatch::group &group,
			delegate_type &delegate
		):
			in_order_streaming_reader(
				std::thread::hardware_concurrency() ?: 1,
				queue,
				group,
				delegate
			)
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

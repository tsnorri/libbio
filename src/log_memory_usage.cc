/*
 * Copyright (c) 2023-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#if defined(LIBBIO_LOG_ALLOCATED_MEMORY) && LIBBIO_LOG_ALLOCATED_MEMORY

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmismatched-new-delete"

#include <atomic>
#include <boost/endian.hpp>
#include <charconv>						// std::from_chars
#include <cstdlib>						// std::malloc, std::free
#include <iostream>
#include <libbio/assert.hh>
#include <libbio/file_handle.hh>
#include <libbio/file_handling.hh>
#include <libbio/log_memory_usage.hh>
#include <libbio/malloc_allocator.hh>
#include <new>
#include <sstream>
#include <thread>						// std::jthread
#include <unistd.h>						// struct stat
#include <vector>

namespace chrono	= std::chrono;
namespace endian	= boost::endian;
namespace lb		= libbio;
namespace ml		= libbio::memory_logger;


namespace libbio::memory_logger::detail {
	
	std::atomic_uint64_t s_current_state{};
	std::atomic_uint64_t s_state_counter{};
}


namespace {
	
	typedef chrono::steady_clock	clock_type;
	
	
	struct allocation_size
	{
		// tag currently unused.
		std::uint64_t size : 48 {0}, tag : 16 {0};
	};

	static_assert(sizeof(allocation_size) <= __STDCPP_DEFAULT_NEW_ALIGNMENT__);
	static_assert(std::is_trivially_destructible_v <allocation_size>);


	constexpr static inline std::size_t const	HEADER_SIZE{__STDCPP_DEFAULT_NEW_ALIGNMENT__};
	constinit static std::atomic_bool			s_should_continue{true};
	constinit static lb::file_handle			s_logging_handle;
	constinit static std::uint64_t				s_logging_interval{1000};
	constinit static std::uint64_t				s_buffer_size{};
	constinit static std::atomic_uint64_t		s_allocated_memory{};
	constinit static clock_type::time_point		s_start_time{};
	static std::jthread							s_logging_thread{};
	
	
	void clean_up()
	{
		s_should_continue.store(false, std::memory_order_release);
		if (s_logging_thread.joinable())
			s_logging_thread.join();
		s_logging_handle.release();
	}


	constexpr inline std::size_t const RECORD_SIZE{sizeof(ml::event)};
	static_assert(sizeof(std::uint64_t) == RECORD_SIZE);
	
	typedef std::uint64_t buffer_value_type;
	typedef std::vector <buffer_value_type, lb::malloc_allocator <buffer_value_type>> buffer_type;
	
	
	void push_and_check(std::uint64_t const value, buffer_type &buffer)
	{
		// We use big endian byte order b.c. it is easier to read with e.g. xxd or Hex Fiend.
		buffer.push_back(endian::native_to_big(value));
		
		if (buffer.size() == s_buffer_size)
		{
			auto const write_amt(buffer.size() * RECORD_SIZE); // Calculate the number of bytes to be written.
			auto const res(::write(s_logging_handle.get(), buffer.data(), write_amt));
			if (!lb::is_equal(write_amt, res))
			{
				std::cerr << "ERROR: Unable to write to allocation log: " << std::strerror(errno) << '\n';
				std::abort();
			}
			
			buffer.clear();
		}
	}
	
	
	void log_allocations()
	{
		std::uint64_t prev_state_index{};
		buffer_type buffer;
		buffer.reserve(s_buffer_size);
		while (s_should_continue.load(std::memory_order_acquire))
		{
			auto const sampling_time(clock_type::now());
			auto const sampling_time_diff(sampling_time - s_start_time);
			buffer_value_type const sampling_time_diff_ms(chrono::duration_cast <chrono::milliseconds>(sampling_time_diff).count());
			
			push_and_check(sampling_time_diff_ms, buffer);
			
			// Add a marker if needed
			{
				auto const current_state_index(ml::detail::s_state_counter.load(std::memory_order_acquire));
				if (prev_state_index != current_state_index)
				{
					prev_state_index = current_state_index;
					auto const current_state(ml::detail::s_current_state.load(std::memory_order_relaxed));
					push_and_check(ml::event::marker_event(current_state), buffer);
				}
			}
			
			// Allocated memory
			{
				auto const allocated_memory(s_allocated_memory.load(std::memory_order_relaxed));
				auto evt(ml::event::allocated_amount_event(allocated_memory));
				evt.mark_last_in_series();
				push_and_check(evt, buffer);
			}
			
			auto const finish_time(clock_type::now());
			auto const time_spent_ms(chrono::duration_cast <chrono::milliseconds>(finish_time - sampling_time).count());
			if (0 <= time_spent_ms && std::uint64_t(time_spent_ms) < s_logging_interval)
			{
				using namespace std::chrono_literals;
				std::this_thread::sleep_for((s_logging_interval - time_spent_ms) * 1ms);
			}
		}
		
		{
			auto const write_amt(buffer.size() * RECORD_SIZE);
			if (!lb::is_equal(write_amt, ::write(s_logging_handle.get(), buffer.data(), buffer.size() * RECORD_SIZE)))
			{
				std::cerr << "ERROR: Unable to write to allocation log: " << std::strerror(errno) << '\n';
				std::abort();
			}
		}
	}


	inline void log_allocation(void *ptr, std::size_t const size)
	{
		allocation_size *as(new (ptr) allocation_size());
		as->size = size;
		s_allocated_memory.fetch_add(size, std::memory_order_relaxed);
	}


	inline void log_deallocation(void *ptr)
	{
		auto *as(static_cast <allocation_size *>(ptr));
		s_allocated_memory.fetch_sub(as->size, std::memory_order_relaxed);
		// allocation_size is trivially destructible and constructed with placement new.
	}
}


namespace libbio {

	void setup_allocated_memory_logging_(ml::header_writer_delegate &delegate)
	{
		s_start_time = clock_type::now();
		
		char const *log_basename_(std::getenv("LIBBIO_ALLOCATION_LOG") ?: "allocations");
		std::string_view const log_basename(log_basename_);
		if (log_basename.contains('/'))
		{
			std::cerr << "ERROR: Refusing to accept a slash in LIBBIO_ALLOCATION_LOG.\n";
			std::abort();
		}

		char const *allocation_interval(std::getenv("LIBBIO_ALLOCATION_LOGGING_INTERVAL"));
		if (allocation_interval)
		{
			auto const res(std::from_chars(allocation_interval, allocation_interval + std::strlen(allocation_interval), s_logging_interval));
			if (std::errc{} != res.ec)
			{
				std::cerr << "ERROR: Unable to parse LIBBIO_ALLOCATION_LOGGING_INTERVAL.\n";
				std::abort();
			}
		}

		std::atexit(&clean_up);
		
		// Open the log file.
		std::stringstream output_path;
		output_path << log_basename;
		output_path << '.';
		output_path << ::getpid();
		output_path << ".log";
		auto const output_path_(output_path.view());
		auto const fd(lb::open_file_for_writing(output_path_.data(), lb::writing_open_mode::CREATE));
		if (-1 == fd)
		{
			std::cerr << "ERROR: Unable to open ‘" << output_path_ << "’ for writing.\n";
			std::abort();
		}

		s_logging_handle = lb::file_handle(fd);
		struct ::stat sb{};
		s_logging_handle.stat(sb);
		s_buffer_size = ((sb.st_blksize / (2 * RECORD_SIZE)) ?: 16U);
		
		{
			ml::header_writer header_writer;
			header_writer.write_header(fd, delegate);
		}
		
		// Start the logging thread.
		s_logging_thread = std::jthread(log_allocations);
		std::cerr << "Logging memory allocations.\n";
	}
}


namespace {

	inline void *do_allocate(std::size_t const size)
	{
		auto * const allocation(static_cast <char *>(std::malloc(HEADER_SIZE + size)));
		if (allocation)
		{
			log_allocation(allocation, size);
			return allocation + HEADER_SIZE;
		}
		
		throw std::bad_alloc{};
	}


	inline void *do_allocate_aligned(std::size_t const size, std::size_t const alignment_)
	{
		libbio_assert(alignment_ && 0 == (alignment_ & (alignment_ - 1)));
		auto const alignment(std::max(alignment_, alignof(allocation_size)));
		auto * const allocation(static_cast <char *>(std::aligned_alloc(alignment, size + alignment)));
		if (allocation)
		{
			log_allocation(allocation - alignment, size);
			return allocation + alignment;
		}

		throw std::bad_alloc{};
	}


	inline void do_deallocate(void *ptr_)
	{
		auto *ptr(static_cast <char *>(ptr_));
		if (ptr)
		{
			log_deallocation(ptr - HEADER_SIZE);
			std::free(ptr - HEADER_SIZE); // does handle nullptr.
		}
	}


	inline void do_deallocate_aligned(void *ptr_, std::size_t const alignment_)
	{
		auto *ptr(static_cast <char *>(ptr_));
		if (ptr)
		{
			auto const alignment(std::max(alignment_, alignof(allocation_size)));
			log_deallocation(ptr - alignment);
			std::free(ptr); // does handle nullptr.
		}
	}
}


void *operator new(std::size_t const size)
{
	return do_allocate(size);
}


void *operator new(std::size_t const size, std::align_val_t const aln)
{
	return do_allocate_aligned(size, std::to_underlying(aln));
}


void *operator new[](std::size_t const size)
{
	return do_allocate(size);
}


void *operator new[](std::size_t const size, std::align_val_t const aln)
{
	return do_allocate_aligned(size, std::to_underlying(aln));
}


void operator delete(void *ptr) noexcept
{
	do_deallocate(ptr);
}


void operator delete(void *ptr, std::align_val_t const aln) noexcept
{
	do_deallocate_aligned(ptr, std::to_underlying(aln));
}


void operator delete[](void *ptr) noexcept
{
	do_deallocate(ptr);
}


void operator delete[](void *ptr, std::align_val_t const aln) noexcept
{
	do_deallocate_aligned(ptr, std::to_underlying(aln));
}

#pragma GCC diagnostic pop

#endif

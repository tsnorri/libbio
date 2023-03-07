/*
 * Copyright (c) 2023 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#if defined(LIBBIO_LOG_ALLOCATED_MEMORY) && LIBBIO_LOG_ALLOCATED_MEMORY

#include <atomic>
#include <boost/endian.hpp>
#include <charconv>					// std::from_chars
#include <cstdlib>					// std::malloc, std::free
#include <iostream>
#include <libbio/assert.hh>
#include <libbio/file_handle.hh>
#include <libbio/file_handling.hh>
#include <new>
#include <sstream>
#include <thread>					// std::jthread
#include <unistd.h>					// struct stat
#include <vector>

namespace chrono	= std::chrono;
namespace endian	= boost::endian;
namespace lb		= libbio;


namespace {

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
	constinit static std::uint64_t				s_logging_interval{};
	constinit static std::uint64_t				s_buffer_size{};
	constinit static std::atomic_uint64_t		s_allocated_memory{};
	static std::jthread							s_logging_thread{};


	void clean_up()
	{
		s_should_continue.store(false, std::memory_order_release);
		if (s_logging_thread.joinable())
			s_logging_thread.join();
		s_logging_handle.release();
	}


	typedef std::uint64_t record_value_type;
	constexpr inline std::size_t const RECORD_ITEM_COUNT{2};
	constexpr inline std::size_t const RECORD_SIZE{RECORD_ITEM_COUNT * sizeof(record_value_type)};


	void log_allocations()
	{
		typedef record_value_type value_type;
		std::vector <value_type> buffer;
		buffer.reserve(s_buffer_size);
		while (s_should_continue.load(std::memory_order_acquire))
		{
			auto const pp(chrono::system_clock::now().time_since_epoch());
			value_type const ms_count(chrono::duration_cast <chrono::milliseconds>(pp).count());
			auto const allocated_memory(s_allocated_memory.load(std::memory_order_relaxed));

			// We use big endian byte order b.c. it is easier to read with e.g. xxd or Hex Fiend.
			buffer.push_back(endian::native_to_big(ms_count));
			buffer.push_back(endian::native_to_big(allocated_memory));

			if (buffer.size() == s_buffer_size)
			{
				auto const res(::write(s_logging_handle.get(), buffer.data(), buffer.size() * sizeof(value_type)));
				if (-1 == res)
				{
					std::cerr << "ERROR: Unable to write to allocation log: " << std::strerror(errno) << '\n';
					std::abort();
				}

				buffer.clear();
			}

			{
				using namespace std::chrono_literals;
				std::this_thread::sleep_for(s_logging_interval * 1ms);
			}
		}

		::write(s_logging_handle.get(), buffer.data(), buffer.size() * sizeof(value_type));
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

	void setup_allocated_memory_logging_()
	{
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
			std::cerr << "ERROR: Unable to open '" << output_path_ << "' for writing.\n";
			std::abort();
		}

		s_logging_handle = lb::file_handle(fd);
		struct ::stat sb{};
		s_logging_handle.stat(sb);
		s_buffer_size = RECORD_ITEM_COUNT * ((sb.st_blksize / RECORD_SIZE) ?: 1U);

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

#endif

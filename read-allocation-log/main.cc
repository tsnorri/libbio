/*
 * Copyright (c) 2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <array>
#include <boost/endian.hpp>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <libbio/log_memory_usage.hh>
#include <unistd.h>

namespace endian	= boost::endian;
namespace ml		= libbio::memory_logger;


namespace {
	
	struct file_handle
	{
		FILE *fp{};
		
		~file_handle() { if (fp) std::fclose(fp); }
		constexpr operator bool() const { return nullptr != fp; }
	};
}


int main(int argc, char **argv)
{
	if (2 != argc)
	{
		std::cerr << "Usage: read_allocations log-path\n";
		std::exit(EXIT_FAILURE);
	}

	char *path{argv[1]};
	file_handle handle{std::fopen(path, "r")};
	if (!handle)
	{
		std::perror("fopen");
		std::exit(EXIT_FAILURE);
	}

	constexpr std::size_t const max_count{64};
	std::array <std::uint64_t, max_count> buffer{};
	bool is_at_start{true};
	while (true)
	{
		auto const count(std::fread(buffer.data(), sizeof(std::uint64_t), max_count, handle.fp));
		if (0 == count)
			break;
		
		std::uint64_t sampling_time_ms{};
		for (std::size_t i(0); i < count; ++i)
		{
			if (is_at_start)
			{
				is_at_start = false;
				sampling_time_ms = endian::big_to_native(buffer[i]);
				continue;
			}
			
			ml::event const event(endian::big_to_native(buffer[i]));
			
			std::cout << (sampling_time_ms / 1000) << '\t';
			event.output_record(std::cout);
			std::cout << '\n';
			
			if (event.is_last_in_series())
				is_at_start = true;
		}
	}

	if (std::ferror(handle.fp))
	{
		std::perror("fread");
		std::exit(EXIT_FAILURE);
	}

	return EXIT_SUCCESS;
}

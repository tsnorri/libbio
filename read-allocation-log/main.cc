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
#include <map>
#include <unistd.h>

namespace endian	= boost::endian;
namespace ml		= libbio::memory_logger;


namespace {
	
	typedef std::map <std::uint64_t, std::string>	state_map;
	
	
	struct file_handle
	{
		FILE *fp{};
		
		~file_handle() { if (fp) std::fclose(fp); }
		constexpr operator bool() const { return nullptr != fp; }
	};
	
	
	struct header_reader_delegate final : public ml::header_reader_delegate
	{
		state_map state_mapping;
		
		void handle_state(ml::header_reader &reader, std::uint64_t const idx, std::string_view const name) override
		{
			state_mapping.emplace(idx, name);
		}
	};
	
	
	struct event_visitor
	{
		state_map const	&state_mapping;
		std::ostream	&os;
		
		void visit_unknown_event(ml::event const evt)
		{
			os << "unknown\tunknown";
		}
		
		void visit_allocated_amount_event(ml::event const evt)
		{
			os << "allocated_amount\t" << evt.event_data();
		}
		
		void visit_marker_event(ml::event const evt)
		{
			os << "marker\t";
			
			auto const it(state_mapping.find(evt.event_data()));
			if (it == state_mapping.end())
			{
				os << "unknown";
				return;
			}
			
			os << it->second;
		}
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
	
	auto const state_mapping([&]{
		header_reader_delegate delegate;
		ml::header_reader reader;
		
		reader.read_header(handle.fp, delegate);
		return delegate.state_mapping;
	}());
	
	event_visitor evt_visitor{state_mapping, std::cout};
	
	std::cout << "SECONDS\tEVENT\tDATA\n";

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
			event.visit(evt_visitor);
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

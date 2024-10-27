/*
 * Copyright (c) 2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_BINARY_PARSING_RANGE_HH
#define LIBBIO_BINARY_PARSING_RANGE_HH

#include <cstddef>
#include <span>
#include <stdexcept>


namespace libbio::binary_parsing {

	struct range
	{
		std::byte const	*it{};
		std::byte const	*end{};

		range(std::byte const *it_, std::byte const *end_):
			it(it_),
			end(end_)
		{
		}

		range(std::byte const *it_, std::size_t size):
			it(it_),
			end(it_ + size)
		{
		}

		std::size_t size() const { return end - it; }
		void seek(std::size_t const size) { if (end < it + size) throw std::out_of_range{"Attempting to seek past the range end"}; it += size; }
		bool empty() const { return it == end; }
		operator bool() const { return !empty(); }
	};

	inline range to_range(std::span <std::byte const> span) { return range{span.data(), span.size()}; }
}

#endif

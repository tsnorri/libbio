/*
 * Copyright (c) 2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_BGZF_BLOCK_HH
#define LIBBIO_BGZF_BLOCK_HH

#include <cstddef>	// std::byte
#include <cstdint>


namespace libbio::bgzf {
	
	struct block
	{
		std::byte const	*compressed_data{};
		std::size_t		compressed_data_size{};
		std::uint32_t	crc32{};
		std::uint32_t	isize{};
	};
}

#endif

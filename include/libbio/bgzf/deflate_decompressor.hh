/*
 * Copyright (c) 2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_BGZF_DEFLATE_DECOMPRESSOR_HH
#define LIBBIO_BGZF_DEFLATE_DECOMPRESSOR_HH

#include <cstddef>
#include <libdeflate.h>
#include <span>


namespace libbio::bgzf::detail {

	struct deflate_decompressor
	{
		struct libdeflate_decompressor	*decompressor{};

		~deflate_decompressor() { libdeflate_free_decompressor(decompressor); }
		void prepare() { decompressor = libdeflate_alloc_decompressor(); }
		std::span <std::byte> decompress(std::span <std::byte const> in, std::span <std::byte> out);
	};
}

#endif

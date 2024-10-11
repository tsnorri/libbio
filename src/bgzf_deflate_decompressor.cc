/*
 * Copyright (c) 2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <libbio/bgzf/deflate_decompressor.hh>


namespace libbio::bgzf::detail {
	
	std::span <std::byte> deflate_decompressor::decompress(std::span <std::byte const> in, std::span <std::byte> out)
	{
		std::size_t bytes_written{};
		auto const res(libdeflate_deflate_decompress(decompressor, in.data(), in.size(), out.data(), out.size(), &bytes_written));
		switch (res)
		{
			case LIBDEFLATE_SUCCESS:
				break;
			case LIBDEFLATE_BAD_DATA:
				throw std::invalid_argument("Unable to process deflated data");
			case LIBDEFLATE_SHORT_OUTPUT:
			case LIBDEFLATE_INSUFFICIENT_SPACE:
				throw std::runtime_error("Ran out of space while decompressing");
		}
	
		return std::span(out.data(), bytes_written);
	}
}

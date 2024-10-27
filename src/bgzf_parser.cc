/*
 * Copyright (c) 2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <cstddef>
#include <cstdint>
#include <libbio/bgzf/block.hh>
#include <libbio/bgzf/parser.hh>
#include <libbio/binary_parsing/range.hh>
#include <libbio/binary_parsing/read_value.hh>
#include <stdexcept>

namespace lb	= libbio;


namespace {

	void check_magic_string(lb::binary_parsing::range &range)
	{
		auto const sp(lb::binary_parsing::take_bytes <4>(range));
		if (! (sp[0] == std::byte{31} && sp[1] == std::byte{139} && sp[2] == std::byte{8} && sp[3] == std::byte{4}))
			throw std::runtime_error("Invalid BGZF magic string");

		return;
	}
}

namespace libbio::bgzf {

	void parser::parse()
	{
		check_magic_string(range());
		range().seek(6); // Ignore some metadata.
		auto const xlen(take <std::uint16_t>());
		adjust_range(
			[xlen](binary_parsing::range &rr){ rr.end = rr.it + xlen; },
			[this, xlen](){
				while (range())
				{
					auto const si1(take <std::uint8_t>());
					auto const si2(take <std::uint8_t>());
					auto const slen(take <std::uint16_t>());

					if ('B' == si1 && 'C' == si2)
					{
						auto const bsize(take <std::uint16_t>());
						target().compressed_data_size = bsize - xlen - 19;
					}
					else
					{
						// Ignore the other fields.
						range().seek(slen);
					}

				}
			}
		);

		target().compressed_data = range().it;
		range().seek(target().compressed_data_size);

		read_field <&block::crc32>();
		read_field <&block::isize>();
	}
}

/*
 * Copyright (c) 2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <libbio/bam/fields.hh>


namespace {
	
	inline void read_hex_value(unsigned char const hex, std::byte &dst)
	{
		if ('0' <= hex && hex <= '9')
			dst |= std::byte(hex - '0');
		else if ('A' <= hex && hex <= 'F')
			dst |= std::byte(hex - 'A' + 0xA);
		else
			throw std::runtime_error("Unexpected hexadecimal number");
	}
}


namespace libbio::bam::fields::detail {
	
	void read_hex_string(binary_parsing::range &range, std::vector <std::byte> &dst)
	{
		// Required to be NUL-terminated (SAMv1 § 4.2.4)
		if (range.it == range.end)
			throw std::runtime_error("Unable to read expected number of bytes from the input");
			
		do
		{
			if (std::byte{} == *range.it)
				break;
			
			std::byte bb{};
			read_hex_value(std::bit_cast <char>(*range.it), bb);
			bb <<= 4;
			++range.it;
			if (range.it == range.end)
				throw std::runtime_error("Unable to read expected number of bytes from the input");
			read_hex_value(std::bit_cast <char>(*range.it), bb);
			++range.it;
			dst.push_back(bb);
		} while (range.it != range.end);
	}
}

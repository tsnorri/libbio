/*
 * Copyright (c) 2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <cstddef>
#include <cstdint>
#include <libbio/bam/header.hh>
#include <libbio/bam/header_parser.hh>
#include <libbio/binary_parsing/range.hh>
#include <libbio/binary_parsing/read_value.hh>
#include <libbio/sam/header.hh>
#include <libbio/sam/input_range.hh>
#include <libbio/sam/reader.hh>
#include <stdexcept>
#include <string_view>

namespace lb	= libbio;


namespace {

	void check_magic_string(lb::binary_parsing::range &rr)
	{
		auto const sp(lb::binary_parsing::take_bytes <4>(rr));
		if (! (sp[0] == std::byte{66} && sp[1] == std::byte{65} && sp[2] == std::byte{77} && sp[3] == std::byte{1}))
			throw std::runtime_error("Invalid BAM magic string");
	};
}


namespace libbio::bam {

	void header_parser::parse()
	{
		check_magic_string(range());

		// text not necessarily NUL-terminated (SAMv1 § 4.2)
		auto const l_text(take <std::uint32_t>());
		target().text.resize(l_text);
		read_field <&header::text>();

		auto const n_ref(take <std::uint32_t>());
		target().reference_sequences.resize(n_ref);
		for_(target().reference_sequences, [this](auto &pp){
			auto const l_name(take <std::uint32_t>());
			pp.target().name.resize(l_name - 1);

			pp.template read_field <&reference_sequence::name>();
			range().seek(1);

			pp.template read_field <&reference_sequence::l_ref>();
		});
	}
}


namespace libbio::bam::detail {

	void read_header(binary_parsing::range &range, header &hh, sam::header &hh_)
	{
		{
			header_parser parser(range, hh);
			parser.parse();
		}

		{
			sam::reader reader;
			sam::character_range range(std::string_view{hh.text.data(), hh.text.size()});
			reader.read_header(hh_, range);
		}
	}
}

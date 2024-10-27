/*
 * Copyright (c) 2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <cstdint>
#include <libbio/bam/fields.hh>
#include <libbio/bam/record_parser.hh>
#include <libbio/binary_parsing/range.hh>
#include <libbio/sam/record.hh>


namespace libbio::bam {

	void record_parser::parse()
	{
		auto const record_size(take <std::uint32_t>());
		adjust_range(
			[record_size](binary_parsing::range &rr){ rr.end = rr.it + record_size; },
			[this](){
				read_field <&sam::record::rname_id>();
				read_field <&sam::record::pos>();
				auto const l_read_name(take <std::uint8_t>());
				read_field <&sam::record::mapq>();
				read_field <&sam::record::bin>();
				auto const n_cigar_op(take <std::uint16_t>());
				read_field <&sam::record::flag>();
				auto const l_seq(take <std::uint32_t>());
				read_field <&sam::record::rnext_id>();
				read_field <&sam::record::pnext>();
				read_field <&sam::record::tlen>();

				target().seq.resize(l_seq);
				target().qual.resize(l_seq);
				target().qname.resize(l_read_name - 1);
				target().cigar.resize(n_cigar_op);

				read_field <&sam::record::qname>();
				range().seek(1); // Skip the NUL byte.

				read_field <&sam::record::cigar, fields::cigar>();
				read_field <&sam::record::seq, fields::seq>();
				read_field <&sam::record::qual, fields::qual>();

				// sam::optional_field is special case in the sense that it needs to be cleared before filling.
				// (std::vector and std::string are overwritten.)
				target().optional_fields.clear();

				while (range())
					read_field <&sam::record::optional_fields, fields::optional>();
			}
		);
	}
}

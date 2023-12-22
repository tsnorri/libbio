/*
 * Copyright (c) 2023 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_SAM_READER_HH
#define LIBBIO_SAM_READER_HH

#include <libbio/sam/cigar_field_parser.hh>
#include <libbio/sam/header.hh>
#include <libbio/sam/input_range.hh>
#include <libbio/sam/optional_field_parser.hh>
#include <libbio/sam/parse_error.hh>


namespace libbio::sam {
	
	class reader
	{
	public:
		typedef parsing::traits::delimited <parsing::delimiter <'\t'>, parsing::delimiter <'\n'>> parser_trait_type;
		typedef parsing::parser <
			parser_trait_type,
			parsing::fields::text <>,						//  0: QNAME, may be *
			parsing::fields::integer <std::uint16_t>,		//  1: FLAG
			parsing::fields::text <>,						//  2: RNAME, may be *
			parsing::fields::integer <std::uint32_t>,		//  3: POS
			parsing::fields::integer <std::uint8_t>,		//  4: MAPQ
			fields::cigar_field,							//  5: CIGAR, may be *
			parsing::fields::text <>,						//  6: RNEXT, may be * or =
			parsing::fields::integer <std::uint32_t>,		//  7: PNEXT
			parsing::fields::integer <std::int32_t>,		//  8: TLEN
			parsing::fields::character_sequence <char>,		//  9: SEQ, may be *
			parsing::fields::character_sequence <char>,		// 10: QUAL, may be *
			fields::optional_field							// 11: Optional fields
		> parser_type;
		
	private:
		void prepare_record(header const &header_, parser_type::record_type &src, record &dst) const;
		void prepare_parser_record(record &src, parser_type::record_type &dst) const;
		
	public:
		void read_header(header &, input_range_base &) const;
		
		template <typename t_range, typename t_cb>
		void read_records(header const &, t_range &&, t_cb &&) const
		requires std::derived_from <std::remove_cvref_t <t_range>, input_range_base>;
		
		template <typename t_cb>
		inline void read_records(header const &header_, file_handle &fh, t_cb &&cb) const;
	};
	
	
	template <typename t_range, typename t_cb>
	void reader::read_records(header const &header_, t_range &&range, t_cb &&cb) const
	requires std::derived_from <std::remove_cvref_t <t_range>, input_range_base>
	{
		parser_type parser;
		parser_type::record_type rec;
		record rec_;
		
		while (parser.parse(range, rec))
		{
			prepare_record(header_, rec, rec_);
			cb(rec_);
			prepare_parser_record(rec_, rec);
		}
	}
	
	
	template <typename t_cb>
	void reader::read_records(header const &header_, file_handle &fh, t_cb &&cb) const
	{
		read_records(header_, file_handle_input_range{fh}, std::forward <t_cb>(cb));
	}
}

#endif

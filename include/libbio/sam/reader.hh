/*
 * Copyright (c) 2023-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_SAM_READER_HH
#define LIBBIO_SAM_READER_HH

#include <libbio/sam/cigar_field_parser.hh>
#include <libbio/sam/header.hh>
#include <libbio/sam/input_range.hh>
#include <libbio/sam/optional_field_parser.hh>
#include <libbio/sam/parse_error.hh>


namespace libbio::sam::detail {
	
	typedef parsing::traits::delimited <
		parsing::delimiter <'\t'>,
		parsing::delimiter <'\n'>
	> parser_trait_type;
	
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
	
	void prepare_record(header const &header_, parser_type::record_type &src, record &dst);
	void prepare_parser_record(record &src, parser_type::record_type &dst);
}


namespace libbio::sam {
	
	template <typename t_record>
	requires std::derived_from <t_record, record>
	class record_reader
	{
	public:
		typedef detail::parser_trait_type	parser_trait_type;
		typedef detail::parser_type			parser_type;
		typedef t_record					record_type;
		
	private:
		parser_type					m_parser;
		parser_type::record_type	m_parser_record;
		record_type					m_record;
		
	public:
		// Valid after calling prepare_one().
		record_type &record() { return m_record; }
		record_type const &record() const { return m_record; }
		
		template <typename t_range>
		bool prepare_one(header const &header_, t_range &&range)
		requires std::derived_from <std::remove_cvref_t <t_range>, input_range_base>
		{
			detail::prepare_parser_record(m_record, m_parser_record);
			if (!m_parser.parse(range, m_parser_record))
				return false;
		
			detail::prepare_record(header_, m_parser_record, m_record);
			return true;
		}
		
		template <typename t_range, typename t_cb>
		void read_all(header const &header_, t_range &&range, t_cb &&cb)
		requires std::derived_from <std::remove_cvref_t <t_range>, input_range_base>
		{
			while (m_parser.parse(range, m_parser_record))
			{
				detail::prepare_record(header_, m_parser_record, m_record);
				cb(m_record);
				detail::prepare_parser_record(m_record, m_parser_record);
			}
		}
	};
	
	
	class reader
	{
	public:
		typedef record_reader <record>		record_reader_type;
		typedef detail::parser_trait_type	parser_trait_type;
		typedef detail::parser_type			parser_type;
		
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
		record_reader_type reader;
		reader.read_all(header_, std::forward <t_range>(range), std::forward <t_cb>(cb));
	}
	
	
	template <typename t_cb>
	void reader::read_records(header const &header_, file_handle &fh, t_cb &&cb) const
	{
		read_records(header_, file_handle_input_range{fh}, std::forward <t_cb>(cb));
	}
}

#endif

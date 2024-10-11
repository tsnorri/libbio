/*
 * Copyright (c) 2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_BAM_HEADER_PARSER_HH
#define LIBBIO_BAM_HEADER_PARSER_HH

#include <libbio/bam/header.hh>
#include <libbio/binary_parsing/endian.hh>
#include <libbio/binary_parsing/parser.hh>
#include <libbio/sam/header.hh>


namespace libbio::bam {
	
	class header_parser final : public binary_parsing::parser_ <header, binary_parsing::endian::little> // RFC 1952 § 2.1
	{
	public:
		using binary_parsing::parser_ <header, binary_parsing::endian::little>::parser_;
		
		void parse() override;
	};
}


namespace libbio::bam::detail {
	
	void read_header(binary_parsing::range &range, header &hh, sam::header &hh_);
}

#endif

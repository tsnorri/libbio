/*
 * Copyright (c) 2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_BAM_RECORD_PARSER_HH
#define LIBBIO_BAM_RECORD_PARSER_HH

#include <libbio/binary_parsing/endian.hh>
#include <libbio/binary_parsing/parser.hh>
#include <libbio/sam/record.hh>


namespace libbio::bam {
	
	class record_parser final : public binary_parsing::parser_ <sam::record, binary_parsing::endian::little> // Endian order from RFC 1952 § 2.1.
	{
	public:
		using binary_parsing::parser_ <sam::record, binary_parsing::endian::little>::parser_;
		
		void parse() override;
	};
}

#endif

/*
 * Copyright (c) 2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_BGZF_PARSER_HH
#define LIBBIO_BGZF_PARSER_HH

#include <libbio/bgzf/block.hh>
#include <libbio/binary_parsing/endian.hh>
#include <libbio/binary_parsing/parser.hh>


namespace libbio::bgzf {
	
	class parser final : public binary_parsing::parser_ <block, binary_parsing::endian::little> // Endian order from RFC 1952 § 2.1.
	{
	public:
		using binary_parsing::parser_ <block, binary_parsing::endian::little>::parser_;
		
		void parse() override;
	};
}

#endif

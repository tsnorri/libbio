/*
 * Copyright (c) 2019 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <catch2/catch.hpp>
#include <libbio/fasta_reader.hh>
#include <sstream>

namespace gen	= Catch::Generators;
namespace lb	= libbio;


namespace {
	
	class delegate : public lb::fasta_reader_delegate
	{
		std::stringstream m_stream;
		
	public:
		std::stringstream const &stream() { return m_stream; }
		
		virtual bool handle_comment_line(lb::fasta_reader &reader, std::string_view const &sv) override
		{
			m_stream << ';' << sv << '\n';
			return true;
		}
		
		virtual bool handle_identifier(lb::fasta_reader &reader, std::string_view const &sv) override
		{
			m_stream << '>' << sv << '\n';
			return true;
		}
		
		virtual bool handle_sequence_line(lb::fasta_reader &reader, std::string_view const &sv) override
		{
			m_stream << sv << '\n';
			return true;
		}
	};
}


SCENARIO("FASTA files can be parsed")
{
	GIVEN("A test file")
	{
		libbio::mmap_handle handle;
		handle.open("test-files/test.fa");
		
		WHEN("the file is parsed")
		{
			lb::fasta_reader reader;
			delegate cb;
			reader.parse(handle, cb);
			
			THEN("the difference size matches the expected")
			{
				auto const expected(handle.to_string_view());
				auto const actual(cb.stream().str());
				REQUIRE(expected == actual);
			}
		}
	}
}

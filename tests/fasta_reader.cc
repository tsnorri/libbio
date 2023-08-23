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
		
		virtual bool handle_identifier(lb::fasta_reader &reader, std::string_view const &identifier, std::vector <std::string_view> const &extra_fields) override
		{
			m_stream << '>' << identifier;
			for (auto const &extra : extra_fields)
				m_stream << '\t' << extra; // For simplicity we should always test with one tab separating the identifier and the extra field.
			m_stream << '\n';
			return true;
		}
		
		virtual bool handle_sequence_line(lb::fasta_reader &reader, std::string_view const &sv) override
		{
			m_stream << sv << '\n';
			return true;
		}
	};
}


SCENARIO("FASTA files can be parsed", "[fasta_reader]")
{
	GIVEN("A test file")
	{
		libbio::mmap_handle <char> handle;
		handle.open("test-files/test.fa");
		
		WHEN("the file is parsed")
		{
			lb::fasta_reader reader;
			delegate cb;
			reader.parse(handle, cb);
			
			THEN("the parsed FASTA matches the expected")
			{
				auto const expected(handle.to_string_view());
				auto const actual(cb.stream().str());
				REQUIRE(expected == actual);
			}
		}
	}
	
	GIVEN("A test file without a terminating newline")
	{
		libbio::mmap_handle <char> handle;
		handle.open("test-files/test-noeol.fa");
		
		WHEN("the file is parsed")
		{
			lb::fasta_reader reader;
			delegate cb;
			reader.parse(handle, cb);
			
			THEN("the parsed FASTA matches the expected")
			{
				auto const expected(handle.to_string_view());
				auto actual(cb.stream().str());
				actual.pop_back();
				REQUIRE(expected == actual);
			}
		}
	}
	
	GIVEN("A test file with a comment in the end without a terminating newline")
	{
		libbio::mmap_handle <char> handle;
		handle.open("test-files/test-noeol-2.fa");
		
		WHEN("the file is parsed")
		{
			lb::fasta_reader reader;
			delegate cb;
			reader.parse(handle, cb);
			
			THEN("the parsed FASTA matches the expected")
			{
				auto const expected(handle.to_string_view());
				auto actual(cb.stream().str());
				actual.pop_back();
				REQUIRE(expected == actual);
			}
		}
	}
	
	GIVEN("A test file with a sequence without a header")
	{
		libbio::mmap_handle <char> handle;
		handle.open("test-files/test-2.fa");
		
		WHEN("the file is parsed")
		{
			lb::fasta_reader reader;
			delegate cb;
			reader.parse(handle, cb);
			
			THEN("the parsed FASTA matches the expected")
			{
				auto const expected(handle.to_string_view());
				auto const actual(cb.stream().str());
				REQUIRE(expected == actual);
			}
		}
	}
	
	GIVEN("A test file with a sequence without a header and a terminating newline")
	{
		libbio::mmap_handle <char> handle;
		handle.open("test-files/test-noeol-3.fa");
		
		WHEN("the file is parsed")
		{
			lb::fasta_reader reader;
			delegate cb;
			reader.parse(handle, cb);
			
			THEN("the parsed FASTA matches the expected")
			{
				auto const expected(handle.to_string_view());
				auto actual(cb.stream().str());
				actual.pop_back();
				REQUIRE(expected == actual);
			}
		}
	}
}

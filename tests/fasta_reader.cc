/*
 * Copyright (c) 2019-2023 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <catch2/catch.hpp>
#include <libbio/fasta_reader.hh>
#include <libbio/file_handling.hh>
#include <libbio/mmap_handle.hh>
#include <sstream>

namespace gen	= Catch::Generators;
namespace lb	= libbio;


namespace {
	
	class delegate final : public lb::fasta_reader_delegate
	{
		std::stringstream	m_stream;
		
	public:
		std::stringstream const &stream() { return m_stream; }
		
		bool handle_comment_line(lb::fasta_reader_base &reader, std::string_view const &sv) override
		{
			m_stream << ';' << sv << '\n';
			return true;
		}
		
		bool handle_identifier(lb::fasta_reader_base &reader, std::string_view const &identifier, std::vector <std::string_view> const &extra_fields) override
		{
			m_stream << '>' << identifier;
			for (auto const &extra : extra_fields)
				m_stream << '\t' << extra; // For simplicity we should always test with one tab separating the identifier and the extra field.
			m_stream << '\n';
			return true;
		}
		
		bool handle_sequence_chunk(lb::fasta_reader_base &reader, std::string_view const &sv, bool has_newline) override
		{
			m_stream << sv;
			if (has_newline)
				m_stream << '\n';
			return true;
		}
		
		bool handle_sequence_end(lb::fasta_reader_base &reader) override
		{
			return true;
		}
	};
}


SCENARIO("FASTA files can be parsed", "[fasta_reader]")
{
	GIVEN("An empty file")
	{
		lb::file_handle handle(lb::open_file_for_reading("test-files/empty.fa"));
		WHEN("the file is parsed")
		{
			lb::fasta_reader reader;
			delegate cb;
			reader.parse(handle, cb);
			
			THEN("the parsed FASTA matches the expected")
			{
				auto const actual(cb.stream().str());
				REQUIRE("" == actual);
			}
		}
	}
	
	GIVEN("A test file")
	{
		lb::file_handle handle(lb::open_file_for_reading("test-files/test.fa"));
		auto handle_(lb::mmap_handle <char>::mmap(handle));
		WHEN("the file is parsed")
		{
			lb::fasta_reader reader;
			delegate cb;
			reader.parse(handle, cb);
			
			THEN("the parsed FASTA matches the expected")
			{
				auto const expected(handle_.to_string_view());
				auto const actual(cb.stream().str());
				REQUIRE(expected == actual);
			}
		}
	}
	
	GIVEN("A test file without a terminating newline")
	{
		lb::file_handle handle(lb::open_file_for_reading("test-files/test-noeol.fa"));
		auto handle_(lb::mmap_handle <char>::mmap(handle));
		WHEN("the file is parsed")
		{
			lb::fasta_reader reader;
			delegate cb;
			reader.parse(handle, cb);
			
			THEN("the parsed FASTA matches the expected")
			{
				auto const expected(handle_.to_string_view());
				auto actual(cb.stream().str());
				actual.pop_back();
				REQUIRE(expected == actual);
			}
		}
	}
	
	GIVEN("A test file with a comment in the end without a terminating newline")
	{
		lb::file_handle handle(lb::open_file_for_reading("test-files/test-noeol-2.fa"));
		auto handle_(lb::mmap_handle <char>::mmap(handle));
		WHEN("the file is parsed")
		{
			lb::fasta_reader reader;
			delegate cb;
			reader.parse(handle, cb);
			
			THEN("the parsed FASTA matches the expected")
			{
				auto const expected(handle_.to_string_view());
				auto actual(cb.stream().str());
				actual.pop_back();
				REQUIRE(expected == actual);
			}
		}
	}
	
	GIVEN("A test file with a sequence without a header")
	{
		lb::file_handle handle(lb::open_file_for_reading("test-files/test-2.fa"));
		auto handle_(lb::mmap_handle <char>::mmap(handle));
		WHEN("the file is parsed")
		{
			lb::fasta_reader reader;
			delegate cb;
			reader.parse(handle, cb);
			
			THEN("the parsed FASTA matches the expected")
			{
				auto const expected(handle_.to_string_view());
				auto const actual(cb.stream().str());
				REQUIRE(expected == actual);
			}
		}
	}
	
	GIVEN("A test file with a sequence without a header and a terminating newline")
	{
		lb::file_handle handle(lb::open_file_for_reading("test-files/test-noeol-3.fa"));
		auto handle_(lb::mmap_handle <char>::mmap(handle));
		WHEN("the file is parsed")
		{
			lb::fasta_reader reader;
			delegate cb;
			reader.parse(handle, cb);
			
			THEN("the parsed FASTA matches the expected")
			{
				auto const expected(handle_.to_string_view());
				auto actual(cb.stream().str());
				actual.pop_back();
				REQUIRE(expected == actual);
			}
		}
	}
	
	GIVEN("A test file with extra header fields")
	{
		lb::file_handle handle(lb::open_file_for_reading("test-files/extra-fields.fa"));
		auto handle_(lb::mmap_handle <char>::mmap(handle));
		WHEN("the file is parsed")
		{
			lb::fasta_reader reader;
			delegate cb;
			reader.parse(handle, cb);
			
			THEN("the parsed FASTA matches the expected")
			{
				auto const expected(handle_.to_string_view());
				auto actual(cb.stream().str());
				actual.pop_back();
				REQUIRE(expected == actual);
			}
		}
	}
}

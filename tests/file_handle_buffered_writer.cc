/*
 * Copyright (c) 2021 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <catch2/catch.hpp>
#include <libbio/buffered_writer/file_handle_buffered_writer.hh>
#include <libbio/file_handling.hh>
#include <libbio/utility.hh>										// libbio::is_equal()

namespace gen	= Catch::Generators;
namespace lb	= libbio;


SCENARIO("file_handle_buffered_writer can write to a file", "[file_handling]")
{
	SECTION("Test 31 characters")
	{
		GIVEN("a sequence of characters")
		{
			std::string const seq("ABCDEFGHIJKLMNOPQRSTUVWXYZabcde");
			std::string path_template("/tmp/libbio_unit_test_XXXXXX"); // FIXME: replace /tmp with the value of an environment variable.
			
			// Create a temporary file.
			lb::file_handle temp_handle((lb::open_temporary_file_for_rw(path_template)));
			
			// Open the same file for reading to prevent it from being unlinked.
			lb::file_handle read_handle(lb::open_file_for_reading(path_template));
			
			
			WHEN("written to a file")
			{
				{
					// Write to the temporary file.
					lb::file_handle_buffered_writer writer(temp_handle.release(), 16);
					REQUIRE(-1 == temp_handle.get());
					
					writer << seq;
				}
				
				THEN("the file contents match the original sequence")
				{
					std::string buffer(seq.size(), 0);
					auto const res(read(read_handle.get(), buffer.data(), buffer.size()));
					
					REQUIRE(lb::is_equal(seq.size(), res));
					REQUIRE(buffer == seq);
				}
			}
		}
	}
}

/*
 * Copyright (c) 2021-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_NO_DISPATCH

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <libbio/buffered_writer/dispatch_io_channel_buffered_writer.hh>
#include <libbio/file_handle.hh>
#include <libbio/file_handling.hh>

namespace gen	= Catch::Generators;
namespace lb	= libbio;


SCENARIO("dispatch_io_channel_buffered_writer can write to a file", "[file_handling]")
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
			
			// Create a queue for handling exceptions.
			// FIXME: an exception handler is needed.
			lb::dispatch_ptr reporting_queue(dispatch_queue_create("fi.iki.tsnorri.libbio.test-reporting-queue", DISPATCH_QUEUE_SERIAL));
			
			WHEN("written to a file")
			{
				{
					// Write to the temporary file.
					lb::dispatch_io_channel_buffered_writer writer(temp_handle.release(), 16, *reporting_queue);
					REQUIRE(-1 == temp_handle.get());
					
					writer << seq;
					
					// The output position should match the buffer size until flush() is called.
					REQUIRE(writer.output_position() == 16);
					REQUIRE(writer.tellp() == seq.size());
				}
				
				THEN("the file contents match the original sequence")
				{
					std::string buffer(seq.size(), 0);
					auto const res(read(read_handle.get(), buffer.data(), buffer.size()));
					
					REQUIRE(seq.size() == res);
					REQUIRE(buffer == seq);
				}
			}
		}
	}
}

#endif

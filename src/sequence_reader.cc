/*
 * Copyright (c) 2017 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <libbio/file_handling.hh>
#include <libbio/sequence_reader/sequence_reader.hh>


namespace libbio { namespace sequence_reader { namespace detail {
	
	void load_list_input(std::istream &stream, std::unique_ptr <sequence_container> &container_ptr)
	{
		auto *container(new multiple_mmap_sequence_container());
		container_ptr.reset(container);
		
		// Read the input file names and handle each file.
		std::string path;
		while (std::getline(stream, path))
			container->open_file(path);
	}
	
	
	void load_line_input(char const *path, std::unique_ptr <sequence_container> &container_ptr)
	{
		auto *container(new mmap_sequence_container());
		container_ptr.reset(container);
		container->open_file(path);
	}
}}}


namespace libbio { namespace sequence_reader {
	
	void read_input_from_path(char const *path, input_format const format, bool prefer_mmap, std::unique_ptr <sequence_container> &container)
	{
		if (prefer_mmap && format == input_format::TEXT)
			detail::load_line_input(path, container);
		else
		{
			file_istream stream;
			open_file_for_reading(path, stream);
			read_input_from_stream(stream, format, container);
		}
		
		container->set_path(path);
	}
	
	
	void read_input_from_stream(std::istream &stream, input_format const format, std::unique_ptr <sequence_container> &container_ptr)
	{
		switch (format)
		{
			case input_format::FASTA:
			{
				auto *container(new vector_sequence_container);
				container_ptr.reset(container);
		
				typedef vector_source <std::vector <std::uint8_t>> vector_source;
				typedef fasta_reader_cb <vector_source> fasta_reader_cb;
				typedef fasta_reader <vector_source, fasta_reader_cb, 0> fasta_reader;
		
				vector_source vs;
				fasta_reader reader;
				fasta_reader_cb cb(container->sequences());
				reader.read_from_stream(stream, vs, cb);
				break;
			}
				
			case input_format::TEXT:
			{
				auto *container(new vector_sequence_container);
				container_ptr.reset(container);
		
				typedef vector_source <std::vector <std::uint8_t>> vector_source;
				typedef line_reader_cb <vector_source> line_reader_cb;
				typedef line_reader <vector_source, line_reader_cb, 0> line_reader;
		
				vector_source vs;
				line_reader reader;
				line_reader_cb cb(container->sequences());
				reader.read_from_stream(stream, vs, cb);
				break;
			}
				
			case input_format::LIST_FILE:
				detail::load_list_input(stream, container_ptr);
				break;
				
			default:
				libbio_fail("Unexpected input format.");
				break;
		}
	}
	
	
	void read_input(char const *path, input_format const format, std::unique_ptr <sequence_container> &container)
	{
		if (!path)
			read_input_from_stream(std::cin, format, container);
		else if ('-' == path[0] && '\0' == path[1])
			read_input_from_stream(std::cin, format, container);
		else
			read_input_from_path(path, format, true, container);
	}
	
	
	void read_list_from_stream(std::istream &stream, std::vector <std::string> &paths)
	{
		paths.clear();
		
		// Read the input file names and handle each file.
		std::string path;
		while (std::getline(stream, path))
			paths.emplace_back(path);
	}
	
	
	void read_list_file(char const *path, std::vector <std::string> &paths)
	{
		if (!path)
			read_list_from_stream(std::cin, paths);
		else if ('-' == path[0] && '\0' == path[1])
			read_list_from_stream(std::cin, paths);
		else
		{
			file_istream stream;
			open_file_for_reading(path, stream);
			read_list_from_stream(stream, paths);
		}
	}
}}

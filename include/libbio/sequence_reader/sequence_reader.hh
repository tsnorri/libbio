/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_SEQUENCE_READER_SEQUENCE_READER_HH
#define LIBBIO_SEQUENCE_READER_SEQUENCE_READER_HH

#include <istream>
#include <libbio/fasta_reader.hh>
#include <libbio/line_reader.hh>
#include <libbio/sequence_reader/reader_cb.hh>
#include <libbio/sequence_reader/sequence_container.hh>
#include <libbio/vector_source.hh>
#include <memory>
#include <vector>


namespace libbio { namespace sequence_reader {
	
	enum class input_format : uint8_t {
		FASTA		= 0,
		TEXT,
		LIST_FILE
	};
}}


namespace libbio { namespace sequence_reader { namespace detail {
	
	void load_list_input(std::istream &stream, std::unique_ptr <sequence_container> &container);
	void load_line_input(char const *path, std::unique_ptr <sequence_container> &container);
}}}



namespace libbio { namespace sequence_reader {
	
	void read_input_from_path(char const *path, input_format const format, bool prefer_mmap, std::unique_ptr <sequence_container> &container);
	void read_input_from_stream(std::istream &stream, input_format const format, std::unique_ptr <sequence_container> &container);
	void read_input(char const *path, input_format const format, std::unique_ptr <sequence_container> &container);
	
	void read_list_from_stream(std::istream &stream, std::vector <std::string> &paths);
	void read_list_file(char const *path, std::vector <std::string> &paths);
}}

#endif

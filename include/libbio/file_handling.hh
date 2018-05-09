/*
 * Copyright (c) 2017-2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_FILE_HANDLING_HH
#define LIBBIO_FILE_HANDLING_HH

#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>


namespace libbio {
	
	typedef boost::iostreams::stream <boost::iostreams::file_descriptor_source>	file_istream;
	typedef boost::iostreams::stream <boost::iostreams::file_descriptor_sink>	file_ostream;
	
	void handle_file_error(char const *fname);
	void open_file_for_reading(char const *fname, file_istream &stream);
	void open_file_for_writing(char const *fname, file_ostream &stream, bool const should_overwrite);
	
	template <typename t_stream, typename t_vector>
	void read_from_stream(t_stream &stream, t_vector &buffer)
	{
		stream.seekg(0, std::ios::end);
		std::size_t const size(stream.tellg());
		stream.seekg(0, std::ios::beg);
		buffer.resize(size);
		auto buffer_data(reinterpret_cast <typename t_stream::char_type *>(buffer.data()));
		stream.read(buffer_data, size);
	}
}

#endif

/*
 * Copyright (c) 2017-2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <libbio/file_handling.hh>


namespace ios	= boost::iostreams;


namespace libbio {
	
	void handle_file_error(char const *fname)
	{
		char const *errmsg(strerror(errno));
		std::cerr << "Got an error while trying to open '" << fname << "': " << errmsg << std::endl;
		exit(EXIT_FAILURE);
	}


	void open_file_for_reading(char const *fname, file_istream &stream)
	{
		int fd(open(fname, O_RDONLY));
		if (-1 == fd)
			handle_file_error(fname);

		ios::file_descriptor_source source(fd, ios::close_handle);
		stream.open(source);
		stream.exceptions(std::istream::badbit | std::istream::failbit);
	}
	
	
	void open_file_for_writing(char const *fname, file_ostream &stream, writing_open_mode const mode)
	{
		auto const flags(
			O_WRONLY |
			(mode & writing_open_mode::CREATE		? O_CREAT : 0) |		// Create if requested.
			(mode & writing_open_mode::OVERWRITE	? O_TRUNC : O_EXCL)		// Truncate if OVERWRITE given, otherwise require that the file does not exist.
		);
		int const fd(open(fname, flags, S_IRUSR | S_IWUSR));
		if (-1 == fd)
			handle_file_error(fname);
		
		ios::file_descriptor_sink sink(fd, ios::close_handle);
		stream.open(sink);
	}
}

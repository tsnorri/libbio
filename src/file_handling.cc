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
		stream.exceptions(std::istream::badbit);
	}
	
	
	void open_file_for_writing(char const *fname, file_ostream &stream, bool const should_overwrite)
	{
		int fd(0);
		if (should_overwrite)
			fd = open(fname, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
		else
			fd = open(fname, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
		
		if (-1 == fd)
			handle_file_error(fname);
		
		ios::file_descriptor_sink sink(fd, ios::close_handle);
		stream.open(sink);
	}
}

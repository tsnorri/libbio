/*
 * Copyright (c) 2017-2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <boost/format.hpp>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <libbio/file_handling.hh>
#include <string>

#ifdef __linux__
#include <linux/limits.h>
#endif


namespace ios	= boost::iostreams;


namespace libbio {
	
	void handle_file_error(char const *fname)
	{
		char const *errmsg(strerror(errno));
		std::cerr << "Got an error while trying to open '" << fname << "': " << errmsg << std::endl;
		std::exit(EXIT_FAILURE);
	}
	
	
	void open_file_for_reading(std::string const &fname, file_istream &stream)
	{
		open_file_for_reading(fname.c_str(), stream);
	}
	
	
	void open_file_for_writing(std::string const &fname, file_ostream &stream, writing_open_mode const mode)
	{
		open_file_for_writing(fname.c_str(), stream, mode);
	}
	
	
	bool try_open_file_for_reading(std::string const &fname, file_istream &stream)
	{
		return try_open_file_for_reading(fname.c_str(), stream);
	}
	
	
	void open_file_for_reading(char const *fname, file_istream &stream)
	{
		int const fd(open_file_for_reading(fname));
		ios::file_descriptor_source source(fd, ios::close_handle);
		stream.open(source);
		stream.exceptions(std::istream::badbit);
	}
	
	
	bool try_open_file_for_reading(char const *fname, file_istream &stream)
	{
		auto const [fd, did_open] = try_open_file_for_reading(fname);
		if (!did_open)
			return false;
		
		ios::file_descriptor_source source(fd, ios::close_handle);
		stream.open(source);
		stream.exceptions(std::istream::badbit);
		return true;
	}
	
	
	int open_file_for_reading(char const *fname)
	{
		int fd(open(fname, O_RDONLY));
		if (-1 == fd)
			handle_file_error(fname);
		return fd;
	}
	
	
	int open_file_for_reading(std::string const &fname)
	{
		return open_file_for_reading(fname.data());
	}
	
	
	std::pair <int, bool> try_open_file_for_reading(char const *fname)
	{
		int fd(open(fname, O_RDONLY));
		if (-1 == fd)
			return std::make_pair(-1, false);
		return std::make_pair(fd, true);
	}
	
	
	int open_temporary_file_for_writing(std::string &path_template)
	{
		int const fd(mkstemp(path_template.data()));
		if (-1 == fd)
			handle_file_error(path_template.data());
		return fd;
	}
	
	
	int open_temporary_file_for_writing(std::string &path_template, int suffixlen)
	{
		int const fd(mkstemps(path_template.data(), suffixlen));
		if (-1 == fd)
			handle_file_error(path_template.data());
		return fd;
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
	
	
	bool get_file_path(int fd, std::string &buffer)
	{
		buffer.resize(1 + PATH_MAX);
		buffer[PATH_MAX] = '\0';

#ifdef __linux__
		auto const readlink_path(boost::str(boost::format("/proc/self/fd/%d") % fd));
		auto const size(readlink(readlink_path.c_str(), buffer.data(), PATH_MAX));
		if (-1 == size)
			return false;
		
		buffer[size] = '\0';
		return true;
#else
		auto const status(fcntl(fd, F_GETPATH, buffer.data()));
		if (-1 == status)
			return false;
		
		auto const pos(buffer.find('\0'));
		if (std::string::npos == pos)
			buffer[PATH_MAX] = '\0';
		else
			buffer[pos] = '\0';
		
		return true;
#endif
	}
}

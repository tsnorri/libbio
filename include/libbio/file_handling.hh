/*
 * Copyright (c) 2017-2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_FILE_HANDLING_HH
#define LIBBIO_FILE_HANDLING_HH

#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>
#include <initializer_list>


namespace libbio {
	
	enum writing_open_mode : std::uint32_t
	{
		NONE		= 0x0,
		CREATE		= 0x1,
		OVERWRITE	= 0x2
	};
	
	inline constexpr writing_open_mode
	make_writing_open_mode(std::initializer_list <std::underlying_type_t <writing_open_mode>> const &list)
	{
		std::underlying_type_t <writing_open_mode> retval{};
		for (auto const val : list)
			retval |= val;
		return static_cast <writing_open_mode>(retval);
	}
	
	
	typedef boost::iostreams::stream <boost::iostreams::file_descriptor_source>	file_istream;
	typedef boost::iostreams::stream <boost::iostreams::file_descriptor_sink>	file_ostream;
	
	void handle_file_error(char const *fname);
	
	int open_file_for_reading(char const *fname);
	void open_file_for_reading(char const *fname, file_istream &stream);
	void open_file_for_reading(std::string const &fname, file_istream &stream);

	bool try_open_file_for_reading(std::string const &fname, file_istream &stream);
	bool try_open_file_for_reading(char const *fname, file_istream &stream);
	std::pair <int, bool> try_open_file_for_reading(char const *fname);
	
	void open_file_for_writing(char const *fname, file_ostream &stream, writing_open_mode const mode);
	void open_file_for_writing(std::string const &fname, file_ostream &stream, writing_open_mode const mode);
	
	bool get_file_path(int fd, std::string &buffer);
	
	
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

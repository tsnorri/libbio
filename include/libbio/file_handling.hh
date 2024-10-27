/*
 * Copyright (c) 2017-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_FILE_HANDLING_HH
#define LIBBIO_FILE_HANDLING_HH

#include <cstddef>
#include <cstdint>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>
#include <initializer_list>
#include <ios>
#include <istream>
#include <libbio/file_handle.hh>
#include <ostream>
#include <string>
#include <type_traits>
#include <utility>


namespace libbio {

	typedef boost::iostreams::stream <boost::iostreams::file_descriptor_source>	file_istream;
	typedef boost::iostreams::stream <boost::iostreams::file_descriptor_sink>	file_ostream;
	typedef boost::iostreams::stream <boost::iostreams::file_descriptor>		file_iostream;
}


namespace libbio::detail {

	template <typename t_stream>
	struct is_file_descriptor_stream : public std::false_type {};

	template <> struct is_file_descriptor_stream <file_istream>  : public std::true_type {};
	template <> struct is_file_descriptor_stream <file_ostream>  : public std::true_type {};
	template <> struct is_file_descriptor_stream <file_iostream> : public std::true_type {};

	template <typename t_stream>
	constexpr static inline bool is_file_descriptor_stream_v = is_file_descriptor_stream <t_stream>::value;


	template <typename t_stream, typename t_category>
	constexpr static inline bool has_iostreams_category_v = std::is_base_of_v <t_category, typename t_stream::category>;


	template <typename t_stream>
	constexpr static inline bool is_fd_input_iostream_v =	is_file_descriptor_stream_v <t_stream> &&
															has_iostreams_category_v <t_stream, boost::iostreams::input>;

	template <typename t_stream>
	constexpr static inline bool is_fd_output_iostream_v =	is_file_descriptor_stream_v <t_stream> &&
															has_iostreams_category_v <t_stream, boost::iostreams::output>;
}


namespace libbio {

	// FIXME: make this enum class, add operator| and operator&.
	enum writing_open_mode : std::uint32_t
	{
		NONE		= 0x0,
		CREATE		= 0x1,
		OVERWRITE	= 0x2
	};

	[[nodiscard]] constexpr inline writing_open_mode
	make_writing_open_mode(std::initializer_list <std::underlying_type_t <writing_open_mode>> const &list)
	{
		std::underlying_type_t <writing_open_mode> retval{};
		for (auto const val : list)
			retval |= val;
		return static_cast <writing_open_mode>(retval);
	}


	void handle_file_error(char const *fname);

	[[nodiscard]] int open_file_for_reading(char const *fname);
	[[nodiscard]] int open_file_for_reading(std::string const &fname);
	[[nodiscard]] std::pair <int, bool> try_open_file_for_reading(char const *fname);

	[[nodiscard]] int open_file_for_writing(char const *fname, writing_open_mode const mode);
	[[nodiscard]] int open_file_for_writing(std::string const &fname, writing_open_mode const mode);

	[[nodiscard]] int open_file_for_rw(char const *fname, writing_open_mode const mode);
	[[nodiscard]] int open_file_for_rw(std::string const &fname, writing_open_mode const mode);
	[[nodiscard]] int open_temporary_file_for_rw(std::string &path_template);
	[[nodiscard]] int open_temporary_file_for_rw(std::string &path_template, int suffixlen);
	void open_file_for_rw(char const *fname, file_iostream &stream, writing_open_mode const mode);
	void open_file_for_rw(std::string const &fname, file_iostream &stream, writing_open_mode const mode);

	bool get_file_path(int fd, std::string &buffer);


	template <typename t_stream>
	void open_file_for_reading(char const *fname, t_stream &stream)
	requires(detail::is_fd_input_iostream_v <t_stream>)
	{
		int const fd(open_file_for_reading(fname));
		stream.open(fd, boost::iostreams::close_handle);
		stream.exceptions(std::istream::badbit);
	}


	template <typename t_stream>
	void open_file_for_reading(std::string const &fname, t_stream &stream)
	requires(detail::is_fd_input_iostream_v <t_stream>)
	{
		open_file_for_reading(fname.c_str(), stream);
	}


	template <typename t_stream>
	bool try_open_file_for_reading(char const *fname, t_stream &stream)
	requires(detail::is_fd_input_iostream_v <t_stream>)
	{
		auto const [fd, did_open] = try_open_file_for_reading(fname);
		if (!did_open)
			return false;

		stream.open(fd, boost::iostreams::close_handle);
		stream.exceptions(std::istream::badbit);
		return true;
	}


	template <typename t_stream>
	bool try_open_file_for_reading(std::string const &fname, t_stream &stream)
	requires(detail::is_fd_input_iostream_v <t_stream>)
	{
		return try_open_file_for_reading(fname.c_str(), stream);
	}


	template <typename t_stream>
	void open_file_for_writing(char const *fname, t_stream &stream, writing_open_mode const mode)
	requires(detail::is_fd_output_iostream_v <t_stream>)
	{
		auto const fd(open_file_for_writing(fname, mode));
		// FIXME: check that anything below does not throw.
		stream.open(fd, boost::iostreams::close_handle);
		stream.exceptions(std::istream::badbit); // FIXME: is this correct?
	}


	template <typename t_stream>
	void open_file_for_writing(std::string const &fname, t_stream &stream, writing_open_mode const mode)
	requires(detail::is_fd_output_iostream_v <t_stream>)
	{
		open_file_for_writing(fname.c_str(), stream, mode);
	}


	// FIXME: add check for seekable input?
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


	inline void open_stream_with_file_handle(file_ostream &stream, file_handle &fh)
	{
		stream.open(fh.get(), boost::iostreams::never_close_handle);
		stream.exceptions(std::ostream::badbit);
	}
}

#endif

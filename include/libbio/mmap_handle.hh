/*
 * Copyright (c) 2017-2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_MMAP_HANDLE_HH
#define LIBBIO_MMAP_HANDLE_HH

#include <libbio/cxxcompat.hh>


namespace libbio {
	
	// Manage a memory-mapped file.
	// Ragel works with pointer ranges rather than streams, so using Boost's mapped_file_sink would add complexity.
	class mmap_handle
	{
	protected:
		char		*m_content;
		std::size_t	m_mapped_size{0};
		
	public:
		mmap_handle() = default;
		mmap_handle(mmap_handle &&other) = default;
		mmap_handle(mmap_handle const &other) = delete;
		~mmap_handle();
		
		void open(int fd);
		void open(char const *path);
		void close();
		
		char const *data() const { return m_content; }
		std::size_t size() const { return m_mapped_size; }
		
		mmap_handle &operator=(mmap_handle const &other) = delete;
		mmap_handle &operator=(mmap_handle &&other) & = default;
		
		std::string_view const to_string_view() const { return std::string_view(m_content, m_mapped_size); }
		std::span <char> const to_span() const { return std::span(m_content, m_mapped_size); }
		
		operator std::string_view() const { return to_string_view(); }
		operator std::span <char>() const { return to_span(); }
	};
}

#endif

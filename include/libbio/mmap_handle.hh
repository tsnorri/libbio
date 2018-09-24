/*
 * Copyright (c) 2017-2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_MMAP_HANDLE_HH
#define LIBBIO_MMAP_HANDLE_HH

#include <libbio/cxxcompat.hh>
#include <string>


namespace libbio {
	
	// Manage a memory-mapped file.
	// Ragel works with pointer ranges rather than streams, so using Boost's mapped_file_sink would add complexity.
	class mmap_handle
	{
		friend std::ostream &operator<<(std::ostream &, mmap_handle const &);
		
	protected:
		std::string	m_path;
		char		*m_content{};
		std::size_t	m_mapped_size{};
		
	public:
		mmap_handle() = default;
		mmap_handle(mmap_handle const &) = delete;
		~mmap_handle();
		
		mmap_handle(mmap_handle &&other):
			m_path(std::move(other.m_path)),
			m_content(other.m_content),
			m_mapped_size(other.m_mapped_size)
		{
			other.m_content = nullptr;
			other.m_mapped_size = 0;
		}
		
		void open(int fd);
		void open(std::string const &path);
		void close();
		
		char const *data() const { return m_content; }
		std::size_t size() const { return m_mapped_size; }
		
		mmap_handle &operator=(mmap_handle const &) = delete;
		inline mmap_handle &operator=(mmap_handle &&other) &;
		
		std::string_view const to_string_view() const { return std::string_view(m_content, m_mapped_size); }
		std::span <char const> const to_span() const { return std::span(m_content, m_mapped_size); }
		
		operator std::string_view() const { return to_string_view(); }
		operator std::span <char const>() const { return to_span(); }
	};
	
	std::ostream &operator<<(std::ostream &stream, mmap_handle const &handle);
	
	inline mmap_handle &mmap_handle::operator=(mmap_handle &&other) &
	{
		m_path = std::move(other.m_path);
		m_content = other.m_content;
		m_mapped_size = other.m_mapped_size;
		other.m_content = nullptr;
		other.m_mapped_size = 0;
		return *this;
	}
}

#endif

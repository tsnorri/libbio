/*
 * Copyright (c) 2017-2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_MMAP_HANDLE_HH
#define LIBBIO_MMAP_HANDLE_HH

#include <fcntl.h>
#include <iostream>
#include <libbio/assert.hh>
#include <libbio/cxxcompat.hh>
#include <stdexcept>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>


namespace libbio {
	
	// Manage a memory-mapped file.
	// Ragel works with pointer ranges rather than streams, so using Boost's mapped_file_sink would add complexity.
	template <typename t_type>
	class mmap_handle
	{
		template <typename T>
		friend std::ostream &operator<<(std::ostream &, mmap_handle <T> const &);
		
	protected:
		std::string	m_path;
		t_type		*m_content{};
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
		
		t_type const *data() const { return m_content; }
		std::size_t size() const { return m_mapped_size; }
		std::size_t byte_size() const { return m_mapped_size * sizeof(t_type); }
		
		mmap_handle &operator=(mmap_handle const &) = delete;
		inline mmap_handle &operator=(mmap_handle &&other) &;
		
		std::basic_string_view <t_type> const to_string_view() const { return std::basic_string_view <t_type>(m_content, m_mapped_size); }
		std::span <t_type> const to_span() const { return std::span(m_content, m_mapped_size); }
		
		operator std::string_view() const { return to_string_view(); }
		operator std::span <t_type>() const { return to_span(); }
	};
	
	
	template <typename t_type>
	std::ostream &operator<<(std::ostream &stream, mmap_handle <t_type> const &handle)
	{
		std::string_view const sv(handle.m_content, std::min(16UL, handle.m_mapped_size));
		stream << "path: '" << handle.m_path << "' mapped size: " << handle.m_mapped_size << " content: '" << sv;
		if (16 < handle.m_mapped_size)
			stream << "…";
		stream << '\'';
		return stream;
	}
	
	
	template <typename t_type>
	auto mmap_handle <t_type>::operator=(mmap_handle <t_type> &&other) & -> mmap_handle &
	{
		m_path = std::move(other.m_path);
		m_content = other.m_content;
		m_mapped_size = other.m_mapped_size;
		other.m_content = nullptr;
		other.m_mapped_size = 0;
		return *this;
	}
	
	
	template <typename t_type>
	mmap_handle <t_type>::~mmap_handle()
	{
		if (m_mapped_size)
		{
			libbio_assert(m_content);
			if (-1 == munmap(m_content, byte_size()))
				std::cerr << "munmap: " << strerror(errno) << std::endl;
		}
	}
	
	
	template <typename t_type>
	void mmap_handle <t_type>::close()
	{
		if (m_mapped_size)
		{
			libbio_assert(m_content);
			if (-1 == munmap(m_content, byte_size()))
				throw std::runtime_error(strerror(errno));
			
			m_mapped_size = 0;
		}
	}
	
	
	template <typename t_type>
	void mmap_handle <t_type>::open(int fd)
	{
		struct stat sb{};
		if (-1 == fstat(fd, &sb))
			throw std::runtime_error(strerror(errno));
		
		if (!S_ISREG(sb.st_mode))
			throw std::runtime_error("Trying to memory map a non-regular file");
		
		m_content = static_cast <t_type *>(mmap(0, sb.st_size, PROT_READ, MAP_FILE | MAP_PRIVATE, fd, 0));
		if (MAP_FAILED == m_content)
			throw std::runtime_error(strerror(errno));
		
		m_mapped_size = sb.st_size / sizeof(t_type);
	}
	
	
	template <typename t_type>
	void mmap_handle <t_type>::open(std::string const &path)
	{
		char const *c_str(path.c_str());
		int fd(::open(c_str, O_RDONLY));
		if (-1 == fd)
			throw std::runtime_error(strerror(errno));
			
		open(fd);
		if (-1 == ::close(fd))
			throw std::runtime_error(strerror(errno));
		
		m_path = path;
	}
}

#endif

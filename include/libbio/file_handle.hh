/*
 * Copyright (c) 2021 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_FILE_HANDLE_HH
#define LIBBIO_FILE_HANDLE_HH

#include <algorithm>
#include <utility>


namespace libbio {
	
	class file_handle
	{
	public:
		typedef int	file_descriptor_type;
		
	protected:
		file_descriptor_type			m_fd{-1};
		
	public:
		file_handle() = default;
		
		// Takes ownership.
		explicit file_handle(file_descriptor_type fd):
			m_fd(fd)
		{
		}
		
		~file_handle();
		
		// Copying not allowed.
		file_handle(file_handle const &) = delete;
		file_handle &operator=(file_handle const &) = delete;
		
		inline file_handle(file_handle &&other);
		inline file_handle &operator=(file_handle &&other);
		
		file_descriptor_type get() const { return m_fd; }
		
		// Releases ownership.
		file_descriptor_type release() { auto const retval(m_fd); m_fd = -1; return retval; }
	};
	
	
	file_handle::file_handle(file_handle &&other)
	{
		using std::swap;
		swap(m_fd, other.m_fd);
	}
	
	
	file_handle &file_handle::operator=(file_handle &&other)
	{
		if (this != &other)
		{
			using std::swap;
			swap(m_fd, other.m_fd);
		}
		return *this;
	}
}

#endif
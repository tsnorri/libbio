/*
 * Copyright (c) 2016-2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_DISPATCH_DISPATCH_PTR_HH
#define LIBBIO_DISPATCH_DISPATCH_PTR_HH

#include <dispatch/dispatch.h>
#include <utility>


namespace libbio {

	template <typename t_dispatch>
	class dispatch_ptr
	{
	protected:
		t_dispatch	m_ptr{};
		
	public:
		dispatch_ptr()
		{
		}
		
		explicit dispatch_ptr(t_dispatch ptr, bool retain = false):
			m_ptr(ptr)
		{
			if (m_ptr && retain)
				dispatch_retain(m_ptr);
		}
		
		~dispatch_ptr()
		{
			if (m_ptr)
				dispatch_release(m_ptr);
		}
		
		dispatch_ptr(dispatch_ptr const &other):
			dispatch_ptr(other.m_ptr, true)
		{
		}
		
		dispatch_ptr(dispatch_ptr &&other):
			m_ptr(other.m_ptr)
		{
			other.m_ptr = nullptr;
		}
		
		bool operator==(dispatch_ptr const &other) const
		{
			return m_ptr == other.m_ptr;
		}
		
		bool operator!=(dispatch_ptr const &other) const
		{
			return !(*this == other);
		}
		
		// std::unique_ptr has explicit operator bool, so we should probably too.
		explicit operator bool() const noexcept
		{
			return (m_ptr ? true : false);
		}
		
		dispatch_ptr &operator=(dispatch_ptr const &other) &
		{
			if (*this != other)
			{
				if (m_ptr)
					dispatch_release(m_ptr);
				
				m_ptr = other.m_ptr;
				dispatch_retain(m_ptr);
			}
			return *this;
		}
		
		dispatch_ptr &operator=(dispatch_ptr &&other) &
		{
			if (*this != other)
			{
				m_ptr = other.m_ptr;
				other.m_ptr = nullptr;
			}
			return *this;
		}
		
		t_dispatch operator*() { return m_ptr; }
		
		void reset(t_dispatch ptr = nullptr, bool retain = false)
		{
			if (m_ptr)
				dispatch_release(m_ptr);
			
			m_ptr = ptr;
			if (retain && m_ptr)
				dispatch_retain(m_ptr);
		}
	};
	
	
	template <typename t_dispatch>
	void swap(dispatch_ptr <t_dispatch> &lhs, dispatch_ptr <t_dispatch> &rhs)
	{
		using std::swap;
		swap(lhs.m_ptr, rhs.m_ptr);
	}
}

#endif

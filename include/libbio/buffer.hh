/*
 * Copyright (c) 2018-2019 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_BUFFER_HH
#define LIBBIO_BUFFER_HH

#include <boost/format.hpp>
#include <cstdlib>
#include <cstring>
#include <memory>


namespace libbio { namespace detail {
	
	// Forward declarations.
	
	template <typename>
	struct malloc_deleter;
	
	template <typename, typename, typename>
	struct buffer_helper;
	
	template <typename t_type>
	t_type *alloc(std::size_t const size);
	
	template <typename t_type>
	t_type *aligned_alloc(std::size_t const size, std::size_t const alignment);
}}


namespace libbio {
	
	class buffer_base
	{
	public:
		struct copy_tag {};
		struct zero_tag {};
		
		struct zero_terminated_tag {};
		
	protected:
		std::size_t		m_size{};
		
	public:
		buffer_base() = default;
		
		buffer_base(std::size_t size):
			m_size(size)
		{
		}
		
		std::size_t size() const { return m_size; }
	};
	
	
	template <typename t_type>
	struct buffer_tpl_base : public buffer_base
	{
		using buffer_base::buffer_base;
		virtual t_type *get() const = 0;
	};
	
	
	template <typename t_type, typename t_default_copy_tag = buffer_base::copy_tag>
	class buffer final : public buffer_tpl_base <t_type>
	{
	public:
		typedef t_type value_type;
		
	protected:
		std::unique_ptr <t_type, detail::malloc_deleter <t_type>>	m_content;
		
	public:
		buffer() = default;
		buffer(buffer &&src) = default;
		buffer &operator=(buffer &&src) & = default;
		
		buffer(t_type *value, std::size_t size):	// Takes ownership.
			buffer_tpl_base <t_type>(size),
			m_content(value)
		{
		}
		
		buffer(std::size_t const size):
			buffer(detail::alloc <t_type>(size), size)
		{
		}
		
		// Enable for char *.
		template <typename t_char_type = std::enable_if_t <std::is_same_v <t_type, char>, char>>
		buffer(t_char_type *value, buffer_base::zero_terminated_tag const &): // Takes ownership.
			buffer(value, std::strlen(value))
		{
		}
		
		template <typename t_copy_tag>
		buffer(t_type *value, std::size_t const size, t_copy_tag const &):
			buffer(detail::alloc <t_type>(size), size)
		{
			detail::buffer_helper <t_type, buffer, t_copy_tag>::copy_to_buffer(value, size, *this);
		}
		
		template <typename t_copy_tag = t_default_copy_tag>
		buffer(buffer_tpl_base <t_type> const &src, t_copy_tag const &copy_tag = t_copy_tag()):
			buffer(src.get(), src.size(), copy_tag)
		{
		}
		
		buffer(buffer const &src): buffer(src.get(), src.size(), t_default_copy_tag()) {}
		
		buffer &operator=(buffer_tpl_base <t_type> const &src) &
		{
			if (this != &src)
			{
				m_content.reset(detail::alloc <t_type>(src.m_size));
				detail::buffer_helper <t_type, buffer, t_default_copy_tag>::copy_to_buffer(src, *this);
			}
			return *this;
		}
		
		virtual t_type *get() const override { return m_content.get(); }
	};
	
	
	// Don’t inherit buffer in order to make use of final.
	template <typename t_type, typename t_default_copy_tag = buffer_base::copy_tag>
	class aligned_buffer final : public buffer_tpl_base <t_type>
	{
	protected:
		std::size_t m_alignment{};
		std::unique_ptr <t_type, detail::malloc_deleter <t_type>>	m_content;
		
	protected:
		aligned_buffer(t_type *value, std::size_t const size, std::size_t const alignment):	// Takes ownership.
			buffer_tpl_base <t_type>(size),
			m_alignment(alignment),
			m_content(value)
		{
		}
		
	public:
		aligned_buffer() = default;
		aligned_buffer(aligned_buffer &&src) = default;
		aligned_buffer &operator=(aligned_buffer &&src) & = default;
		
		aligned_buffer(std::size_t const size, std::size_t const alignment):
			aligned_buffer(detail::aligned_alloc <t_type>(size, alignment), size, alignment)
		{
		}
		
		template <typename t_copy_tag>
		aligned_buffer(t_type *value, std::size_t const size, std::size_t const alignment, t_copy_tag const &):
			aligned_buffer(detail::aligned_alloc <t_type>(size, alignment), size, alignment)
		{
			detail::buffer_helper <t_type, aligned_buffer, t_copy_tag>::copy_to_buffer(value, size, *this);
		}
		
		template <typename t_copy_tag = t_default_copy_tag>
		aligned_buffer(buffer_tpl_base <t_type> const &src, std::size_t const alignment, t_copy_tag const &copy_tag = t_copy_tag()):
			aligned_buffer(src.get(), src.m_size, alignment, copy_tag)
		{
		}
		
		template <typename t_copy_tag = t_default_copy_tag>
		aligned_buffer(aligned_buffer const &src, t_copy_tag const &copy_tag = t_copy_tag()):
			aligned_buffer(src.m_content.get(), src.m_size, src.m_alignment, copy_tag)
		{
		}
		
		aligned_buffer(aligned_buffer const &src): aligned_buffer(src, t_default_copy_tag()) {}
		
		aligned_buffer &operator=(aligned_buffer const &src) &
		{
			if (this != &src)
			{
				m_alignment = src.m_alignment;
				m_content.reset(detail::aligned_alloc <t_type>(src.m_size, src.m_alignment));
				detail::buffer_helper <t_type, aligned_buffer, t_default_copy_tag>::copy_to_buffer(src, *this);
			}
			return *this;
		}
		
		void realloc(std::size_t const size, std::size_t const alignment)
		{
			m_content.reset(detail::aligned_alloc <t_type>(size, alignment));
			this->m_size = size;
			m_alignment = alignment;
		}
		
		std::size_t alignment() const { return m_alignment; }
		virtual t_type *get() const override { return m_content.get(); }
	};
	
	
	// Non-owning buffer.
	template <typename t_type>
	class transient_buffer final : public buffer_tpl_base <t_type>
	{
		template <typename>
		friend class buffer;
		
	protected:
		t_type *m_content{};
		
	public:
		transient_buffer() = default;
		
		transient_buffer(t_type *value, std::size_t size): // Does not take ownership.
			buffer_tpl_base <t_type>(size),
			m_content(value)
		{
		}
		
		explicit transient_buffer(buffer_tpl_base <t_type> const &buffer):
			buffer_tpl_base <t_type>(buffer.m_size),
			m_content(buffer.m_content)
		{
		}
		
		virtual t_type *get() const override { return m_content; }
	};
}


namespace libbio { namespace detail {
	
	template <typename t_type>
	struct malloc_deleter
	{
		void operator()(t_type *ptr) const { std::free(ptr); }
	};
	
	
	template <typename t_type>
	t_type *alloc(std::size_t const size)
	{
		auto *retval(reinterpret_cast <t_type *>(std::malloc(size * sizeof(t_type))));
		if (nullptr == retval)
			throw std::bad_alloc();
		return retval;
	}
	
	template <typename t_type>
	t_type *aligned_alloc(std::size_t const size, std::size_t const alignment)
	{
		void *retval{};
#ifdef __APPLE__ // Apparently macOS does not have aligned_alloc.
		if (size)
		{
			auto const st(posix_memalign(&retval, alignment, size * sizeof(t_type)));
			if (0 != st)
				throw std::bad_alloc();
		}
#else
		if (size)
		{
			retval = std::aligned_alloc(alignement, size * sizeof(t_type));
			if (!retval)
				throw std::bad_alloc();
		}
#endif
		return reinterpret_cast <t_type *>(retval);
	}
	
	
	template <typename t_type, typename t_dst, typename t_copy_tag>
	struct buffer_helper_base
	{
	};
	
	template <typename t_type, typename t_dst>
	struct buffer_helper_base <t_type, t_dst, buffer_base::copy_tag>
	{
		static void copy(t_type const *src, std::size_t const size, t_dst &dst)
		{
			std::copy(src, src + size, dst.get());
		}
	};
	
	template <typename t_type, typename t_dst>
	struct buffer_helper_base <t_type, t_dst, buffer_base::zero_tag>
	{
		static void copy_to_buffer(t_type const *, std::size_t const, t_dst &dst)
		{
			std::fill(dst.get(), dst.get() + dst.size(), t_type(0));
		}
	};
	
	template <typename t_type, typename t_dst, typename t_copy_tag>
	struct buffer_helper : public buffer_helper_base <t_type, t_dst, t_copy_tag>
	{
		using buffer_helper_base <t_type, t_dst, t_copy_tag>::copy_to_buffer;
		
		static void copy_to_buffer(buffer_tpl_base <t_type> const &src, t_dst &dst)
		{
			copy_to_buffer(src.get(), src.size(), dst);
		}
	};
}}

#endif

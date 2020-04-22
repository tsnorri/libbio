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
		
	protected:
		std::size_t		m_size{};
		
	public:
		buffer_base() = default;
		
		explicit buffer_base(std::size_t size):
			m_size(size)
		{
		}
		
		buffer_base(buffer_base const &) = default;
		buffer_base &operator=(buffer_base const &) & = default;
		
		buffer_base(buffer_base &&other):
			m_size(other.m_size)
		{
			other.m_size = 0;
		}
		
		buffer_base &operator=(buffer_base &&other) &
		{
			if (this != &other)
			{
				m_size = other.m_size;
				other.m_size = 0;
			}
			return *this;
		}
		
		std::size_t size() const { return m_size; }
	};
	
	
	template <typename t_type>
	struct buffer_tpl_base : public buffer_base
	{
		template <typename, typename>
		friend class buffer;
		
		using buffer_base::buffer_base;
		virtual t_type *get() const = 0;
		virtual t_type *operator*() const = 0;
	};
	
	
	template <typename t_type, typename t_default_copy_tag = buffer_base::copy_tag>
	class buffer final : public buffer_tpl_base <t_type>
	{
	public:
		typedef t_type value_type;
		
	protected:
		typedef buffer_tpl_base <t_type>	base_class;
		
	protected:
		std::unique_ptr <t_type, detail::malloc_deleter <t_type>>	m_content;
		
	protected:
		buffer &copy_from(base_class const &src)
		{
			if (this != &src)
			{
				// Reallocate only if src cannot be copied.
				if (this->m_size < src.m_size)
				{
					m_content.reset(detail::alloc <t_type>(src.m_size));
					base_class::operator=(src); // Sets m_size.
				}
				detail::buffer_helper <t_type, buffer, t_default_copy_tag>::copy_to_buffer(src, *this);
			}
			return *this;
		}
		
	public:
		buffer() = default;
		buffer(buffer &&src) = default;
		buffer &operator=(buffer &&src) & = default;
		
		buffer(t_type *value, std::size_t size):	// Takes ownership.
			base_class(size),
			m_content(value)
		{
		}
		
		explicit buffer(std::size_t const size):
			buffer(detail::alloc <t_type>(size), size)
		{
		}
		
		// Copy constructor.
		template <typename t_copy_tag>
		buffer(t_type *value, std::size_t const size, t_copy_tag const &):
			buffer(detail::alloc <t_type>(size), size)
		{
			detail::buffer_helper <t_type, buffer, t_copy_tag>::copy_to_buffer(value, size, *this);
		}
		
		// Copy constructor.
		template <typename t_copy_tag = t_default_copy_tag>
		buffer(base_class const &src, t_copy_tag const &copy_tag = t_copy_tag()):
			buffer(src.get(), src.size(), copy_tag)
		{
		}
		
		// Copy constructor.
		buffer(buffer const &src): buffer(src.get(), src.size(), t_default_copy_tag()) {}
		
		buffer &operator=(base_class const &src) &
		{
			return copy_from(src);
		}
		
		buffer &operator=(buffer const &src) &
		{
			return copy_from(src);
		}
		
		static buffer buffer_with_allocated_buffer(char *value) // Takes ownership.
		{
			return buffer(value, std::strlen(value));
		}
		
		virtual t_type *get() const override { return m_content.get(); }
		virtual t_type *operator*() const override { return get(); }
	};
	
	
	// Don’t inherit buffer in order to make use of final.
	template <typename t_type, typename t_default_copy_tag = buffer_base::copy_tag>
	class aligned_buffer final : public buffer_tpl_base <t_type>
	{
	public:
		typedef t_type	value_type;
		
	protected:
		typedef buffer_tpl_base <t_type>	base_class;
		
	protected:
		std::size_t m_alignment{};
		std::unique_ptr <t_type, detail::malloc_deleter <t_type>>	m_content;
		
	protected:
		aligned_buffer(t_type *value, std::size_t const size, std::size_t const alignment):	// Takes ownership.
			base_class(size),
			m_alignment(alignment),
			m_content(value)
		{
		}
		
	public:
		aligned_buffer() = default;
		aligned_buffer(aligned_buffer &&src) = default;
		aligned_buffer &operator=(aligned_buffer &&src) & = default;
		
		explicit aligned_buffer(std::size_t const size, std::size_t const alignment = alignof(t_type)):
			aligned_buffer(detail::aligned_alloc <t_type>(size, alignment), size, alignment)
		{
		}
		
		// Copy constructor.
		template <typename t_copy_tag>
		aligned_buffer(t_type *value, std::size_t const size, std::size_t const alignment, t_copy_tag const &):
			aligned_buffer(detail::aligned_alloc <t_type>(size, alignment), size, alignment)
		{
			detail::buffer_helper <t_type, aligned_buffer, t_copy_tag>::copy_to_buffer(value, size, *this);
		}
		
		// Copy constructor.
		template <typename t_copy_tag = t_default_copy_tag>
		aligned_buffer(base_class const &src, std::size_t const alignment, t_copy_tag const &copy_tag = t_copy_tag()):
			aligned_buffer(src.get(), src.m_size, alignment, copy_tag)
		{
		}
		
		// Copy constructor.
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
				// Reallocate only if src cannot be copied.
				if (m_alignment < src.m_alignment || this->m_size < src.m_size)
				{
					m_content.reset(detail::aligned_alloc <t_type>(src.m_size, src.m_alignment));
					base_class::operator=(src); // Sets m_size.
					m_alignment = src.m_alignment;
				}
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
		virtual t_type *operator*() const override { return get(); }
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
			if (alignment <= sizeof(void *))
			{
				retval = malloc(size * sizeof(t_type));
				if (!retval)
					throw std::bad_alloc();
			}
			else
			{
				// posix_memalign requires that the alignment is a power of two and gte. sizeof(void *).
				// FIXME: check that the alignment is actually a power of two.
				auto const st(posix_memalign(&retval, alignment, size * sizeof(t_type))); // retval may be passed to free().
				if (0 != st)
					throw std::bad_alloc();
			}
		}
#else
		if (size)
		{
			retval = std::aligned_alloc(alignment, size * sizeof(t_type)); // retval may be passed to free().
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
		static void copy_to_buffer(t_type const *src, std::size_t const size, t_dst &dst)
		{
			std::copy(src, src + size, dst.get());
		}
	};
	
	template <typename t_type, typename t_dst>
	struct buffer_helper_base <t_type, t_dst, buffer_base::zero_tag>
	{
		static void copy_to_buffer(t_type const *, std::size_t const, t_dst &dst)
		{
			std::fill_n(dst.get(), dst.size(), t_type(0));
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

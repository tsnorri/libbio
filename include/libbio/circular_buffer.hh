/*
 * Copyright (c) 2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_CIRCULAR_BUFFER_HH
#define LIBBIO_CIRCULAR_BUFFER_HH

#include <libbio/assert.hh>
#include <libbio/binary_parsing/range.hh>
#include <libbio/mmap_handle.hh>
#include <type_traits>


namespace libbio::detail {
	
	template <bool t_is_const>
	struct range_tpl
	{
		typedef std::conditional_t <t_is_const, std::byte const *, std::byte *>	pointer_type;
		
		pointer_type	it{};
		pointer_type	end{};
		
		range_tpl() = default;
		
		explicit range_tpl(pointer_type it_, std::size_t const size):
			it(it_),
			end(it_ + size)
		{
		}
		
		std::size_t size() const { return end - it; }
		explicit operator binary_parsing::range() { return {it, end}; }
	};
}


namespace libbio {
	
	class circular_buffer
	{
	public:
		typedef detail::range_tpl <false>	range;
		typedef detail::range_tpl <true>	const_range;
		
	private:
		static int		s_page_size;
		
		mmap_handle		m_handle;
		std::size_t		m_size{};	// Could get this from m_handle with one right shift
		std::uintptr_t	m_mask{};	// Could get this from m_size with one subtraction.
		std::uintptr_t	m_begin{};
		std::uintptr_t	m_end{};
		std::byte		*m_base{};
		
	public:
		circular_buffer() = default;
		
		explicit circular_buffer(std::size_t page_count) { allocate(page_count); }
		
		static int page_size() { return s_page_size; }
		
		std::size_t size() const { return m_size; }
		std::size_t size_occupied() const { return m_end - m_begin; }
		std::size_t size_available() const { return m_size - (m_end - m_begin); }
		
		void add_to_occupied(std::size_t size) { libbio_assert_lte(size, size_available()); m_end += size; }
		void add_to_available(std::size_t size) { libbio_assert_lte(size, size_occupied()); m_begin += size; }
		
		void clear() { m_begin = m_end; }
		
		std::uintptr_t linearise_(std::uintptr_t pos) const { return pos & m_mask; }
		std::byte *linearise(std::uintptr_t pos) { return m_base + linearise_(pos); }
		std::byte const *linearise(std::uintptr_t pos) const { return m_base + linearise_(pos); }
		
		void set_begin(std::byte const *it) { auto const pos(linearise_(it - m_base)); m_begin = pos; }
		
		void allocate(std::size_t page_count);
		const_range reading_range() const { return const_range{linearise(m_begin), size_occupied()}; }
		range writing_range() { return range{linearise(m_end), size_available()}; }
	};
}

#endif
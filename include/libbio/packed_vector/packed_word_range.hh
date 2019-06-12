/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_PACKED_VECTOR_PACKED_WORD_RANGE_HH
#define LIBBIO_PACKED_VECTOR_PACKED_WORD_RANGE_HH

#include <boost/range/algorithm/for_each.hpp>
#include <libbio/algorithm.hh>
#include <libbio/packed_vector/packed_vector.hh>


namespace libbio { namespace detail {
	
	template <typename t_vector>
	class packed_word_range
	{
	public:
		typedef t_vector									vector_type;
		typedef typename vector_type::word_type				word_type;
		typedef packed_vector_iterator_traits <t_vector>	traits;
		typedef typename traits::iterator					iterator;
		typedef typename traits::word_iterator				word_iterator;
		typedef boost::iterator_range <iterator>			iterator_range;
		typedef boost::iterator_range <word_iterator>		word_iterator_range;
		
		enum {
			ELEMENT_COUNT	= vector_type::ELEMENT_COUNT,
			ELEMENT_BITS	= vector_type::ELEMENT_BITS,
			WORD_BITS		= vector_type::WORD_BITS
		};
		
	protected:
		word_iterator_range	m_mid;
		iterator_range		m_left_extent;
		iterator_range		m_right_extent;
		
	public:
		packed_word_range() = default;
		packed_word_range(iterator begin, iterator end);
		
		word_iterator_range mid() { return m_mid; }
		iterator_range left_extent() { return m_left_extent; }
		iterator_range right_extent() { return m_right_extent; }
		template <typename t_unary_fn> void apply_aligned(t_unary_fn &&unary_fn, std::memory_order = std::memory_order_seq_cst) const;
		template <typename t_word_fn, typename t_extent_fn> void apply_parts(t_word_fn &&word_fn, t_extent_fn &&extent_fn);
	};

	
	template <typename t_vector>
	packed_word_range <t_vector>::packed_word_range(iterator begin, iterator end)
	{
		if (begin.word_index() == end.word_index())
		{
			// The range is inside one word.
			auto mid_it(begin.to_containing_word_iterator() + 1);
			m_mid = word_iterator_range(mid_it, mid_it);
			m_left_extent = iterator_range(begin, end);
		}
		else
		{
			// The range covers more than one word.
			auto const begin_idx(begin.index());
			auto const end_idx(end.index());
			
			iterator e1_end(begin);
			iterator e2_begin(end);
			
			auto const rem_1(begin_idx % ELEMENT_COUNT);
			if (0 != rem_1)
				e1_end += (ELEMENT_COUNT - rem_1);
			
			auto const rem_2(end_idx % ELEMENT_COUNT);
			e2_begin -= rem_2;
			
			m_mid = word_iterator_range(e1_end.to_word_iterator(), e2_begin.to_word_iterator());
			m_left_extent = iterator_range(begin, e1_end);
			m_right_extent = iterator_range(e2_begin, end);
		}
	}
	
	
	template <typename t_vector>
	template <typename t_word_fn, typename t_extent_fn>
	void packed_word_range <t_vector>::apply_parts(t_word_fn &&word_fn, t_extent_fn &&extent_fn)
	{
		// Handle the left extent if not empty.
		if (!m_left_extent.empty())
		{
			libbio_assert(
				m_left_extent.begin().word_index() == m_left_extent.end().word_index() ||
				m_left_extent.begin().word_index() == (m_left_extent.end() - 1).word_index()
			);
			
			auto const offset(m_left_extent.begin().word_offset());
			auto const length((m_left_extent.end().word_offset() ?: ELEMENT_COUNT) - offset);
			extent_fn(*(m_mid.begin() - 1), offset * ELEMENT_BITS, length * ELEMENT_BITS);
		}
		
		// Call fn with the middle words if needed.
		for (auto &atomic : m_mid)
			word_fn(atomic);
		
		// Handle the right extent if not empty.
		if (!m_right_extent.empty())
		{
			libbio_assert(m_right_extent.begin().word_index() == m_right_extent.end().word_index());
			
			auto const offset(m_right_extent.begin().word_offset());
			auto const length(m_right_extent.end().word_offset() - offset);
			extent_fn(*(m_mid.end()), offset * ELEMENT_BITS, length * ELEMENT_BITS);
		}
	}
	
	
	template <typename t_vector>
	template <typename t_unary_fn>
	void packed_word_range <t_vector>::apply_aligned(t_unary_fn &&unary_fn, std::memory_order order) const
	{
		// Call unary_fn with each word s.t. the bits are word aligned.
		if (m_left_extent.empty())
		{
			// Already aligned, just handle the last word separately if needed.
			boost::for_each(m_mid, [&unary_fn, order](auto const &val){ unary_fn(val.load(order), ELEMENT_COUNT); });
			if (!m_right_extent.empty())
			{
				auto const size(m_right_extent.size());
				auto const bits(size * ELEMENT_BITS);
				libbio_assert(bits < WORD_BITS);
				
				// end() is not past-the-end b.c. right_extent is not empty.
				word_type const mask((word_type(0x1) << bits) - 1);
				auto last_word(m_mid.end()->load(order) & mask);
				unary_fn(last_word, size);
			}
		}
		else
		{
			// Not aligned, shift and call the function.
			// m_mid.begin() is valid b.c. left extent is not empty.
			auto const left_size(m_left_extent.size());
			auto const left_bits(left_size * ELEMENT_BITS);
			libbio_assert(left_bits < WORD_BITS);
			auto word((m_mid.begin() - 1)->load(order));
			word >>= (m_left_extent.begin().word_offset() * ELEMENT_BITS);
			for (auto const &atomic : m_mid)
			{
				auto const next_word(atomic.load(order));
				word |= next_word << left_bits;
				unary_fn(word, ELEMENT_COUNT);
				word = next_word >> (WORD_BITS - left_bits);
			}
			
			if (m_right_extent.empty())
			{
				// Just after the else before, we checked that left_bits < WORD_BITS. Hence, shifting is safe.
				word_type const mask((word_type(0x1) << left_bits) - 1);
				unary_fn(word & mask, left_size);
			}
			else
			{
				auto const right_size(m_right_extent.size());
				auto const right_bits(right_size * ELEMENT_BITS);
				libbio_assert(right_bits < WORD_BITS);

				// m_mid.end() is not past-the-end b.c. right_extent is not empty.
				word_type const mask((word_type(0x1) << right_bits) - 1);
				auto last_word(m_mid.end()->load(order) & mask);
				word |= last_word << left_bits;
				unary_fn(word, min_ct(ELEMENT_COUNT, left_size + right_size));
				
				if (WORD_BITS - left_bits < right_bits)
					unary_fn(last_word >> (WORD_BITS - left_bits), left_size + right_size - ELEMENT_COUNT);
			}
		}
	}
}}

#endif

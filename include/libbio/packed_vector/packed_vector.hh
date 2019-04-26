/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_PACKED_VECTOR_PACKED_VECTOR_HH
#define LIBBIO_PACKED_VECTOR_PACKED_VECTOR_HH

#include <atomic>
#include <climits>
#include <cmath>
#include <libbio/assert.hh>
#include <libbio/packed_vector/iterator.hh>
#include <range/v3/algorithm/copy.hpp>
#include <range/v3/view/transform.hpp>


namespace libbio { namespace detail {
	
	template <typename t_vector>
	class packed_vector_value_reference
	{
	public:
		typedef typename t_vector::word_type	word_type;
		
	protected:
		t_vector			*m_vector{};
		std::size_t			m_idx{};
		
	public:
		packed_vector_value_reference() = default;
		
		packed_vector_value_reference(t_vector &vector, std::size_t idx):
			m_vector(&vector),
			m_idx(idx)
		{
		}
		
	public:
		word_type load(std::memory_order order = std::memory_order_seq_cst) const { return m_vector->load(m_idx, order); }
		word_type fetch_or(word_type val, std::memory_order const order = std::memory_order_seq_cst) { return m_vector->fetch_or(m_idx, val, order); }
		word_type fetch_and(word_type val, std::memory_order const order = std::memory_order_seq_cst) { return m_vector->fetch_and(m_idx, val, order); }
		
		operator word_type() const { return load(); }
	};
	
	
	template <typename t_vector>
	class packed_vector_word_iterator_proxy
	{
	public:
		typedef typename packed_vector_iterator_traits <t_vector>::word_iterator	word_iterator;

	protected:
		t_vector	*m_vector{};
		
	public:
		packed_vector_word_iterator_proxy() = default;
		packed_vector_word_iterator_proxy(t_vector &vec):
			m_vector(&vec)
		{
		}
		
		word_iterator begin() const { return m_vector->word_begin(); }
		word_iterator end() const { return m_vector->word_end(); }
	};
}}


namespace libbio {
	
	template <unsigned int t_bits, typename t_word = std::uint64_t>
	class packed_vector
	{
		static_assert(std::is_unsigned_v <t_word>);
		
	public:
		typedef t_word		word_type;
		enum { WORD_BITS = CHAR_BIT * sizeof(word_type) };
		
		static_assert(t_bits <= WORD_BITS);
		static_assert(0 == t_bits || 0 == WORD_BITS % t_bits);
		
	protected:
		typedef std::vector <std::atomic <word_type>>							value_vector;
		
	public:
		typedef typename value_vector::reference								word_reference;
		typedef typename value_vector::iterator									word_iterator;
		typedef typename value_vector::const_iterator							const_word_iterator;
		typedef detail::packed_vector_word_iterator_proxy <packed_vector>		word_iterator_proxy;
		typedef detail::packed_vector_word_iterator_proxy <packed_vector const>	const_word_iterator_proxy;

		
		typedef detail::packed_vector_value_reference <packed_vector>			reference_proxy;
		typedef reference_proxy													reference;
		typedef word_type														const_reference;
		
		typedef detail::packed_vector_iterator <packed_vector>					iterator;
		typedef detail::packed_vector_iterator <packed_vector const>			const_iterator;
		
		enum {
			ELEMENT_BITS		= t_bits,
			ELEMENT_COUNT		= WORD_BITS / t_bits,
			ELEMENT_MASK		= (std::numeric_limits <word_type>::max() >> (WORD_BITS - t_bits))
		};
	
	protected:
		value_vector	m_values;
		std::size_t		m_size{};
		
	public:
		packed_vector() = default;
		explicit packed_vector(std::size_t const size):
			m_values(std::ceil(1.0 * size / ELEMENT_COUNT)),
			m_size(size)
		{
		}
		
		// Primitives.
		word_type load(std::size_t const idx, std::memory_order = std::memory_order_seq_cst) const;
		word_type fetch_or(std::size_t const idx, word_type val, std::memory_order const = std::memory_order_seq_cst);
		word_type fetch_and(std::size_t const idx, word_type val, std::memory_order const = std::memory_order_seq_cst);
		
		word_type operator()(std::size_t const idx, std::memory_order order = std::memory_order_seq_cst) const { return load(idx, order); }
		reference_proxy operator()(std::size_t const idx) { return reference_proxy(*this, idx); }
		word_type word_at(std::size_t const idx) const { return m_values[idx]; }
		word_reference word_at(std::size_t const idx) { return m_values[idx]; }
		
		std::size_t size() const { return m_size; }											// Size in elements.
		std::size_t available_size() const { return m_values.size() * ELEMENT_COUNT; }
		std::size_t word_size() const { return m_values.size(); }
		void set_size(std::size_t new_size) { libbio_assert(new_size <= available_size()); m_size = new_size; }
		
		constexpr std::size_t word_bits() const { return WORD_BITS; }
		constexpr std::size_t element_bits() const { return t_bits; }
		constexpr std::size_t element_count_in_word() const { return ELEMENT_COUNT; }
		constexpr word_type element_mask() const { return ELEMENT_MASK; }
		
		// Iterators to packed values.
		iterator begin() { return iterator(*this, 0); }
		iterator end() { return iterator(*this, m_size); }
		const_iterator begin() const { return const_iterator(*this, 0); }
		const_iterator end() const { return const_iterator(*this, m_size); }
		const_iterator cbegin() const { return const_iterator(*this, 0); }
		const_iterator cend() const { return const_iterator(*this, m_size); }
		
		// Iterators to whole words.
		word_iterator word_begin() { return m_values.begin(); }
		word_iterator word_end() { return m_values.end(); }
		const_word_iterator word_begin() const { return m_values.begin(); }
		const_word_iterator word_end() const { return m_values.end(); }
		const_word_iterator word_cbegin() const { return m_values.cbegin(); }
		const_word_iterator word_cend() const { return m_values.cend(); }
		word_iterator_proxy word_range() { return word_iterator_proxy(*this); }
		const_word_iterator_proxy word_range() const { return const_word_iterator_proxy(*this); }
		const_word_iterator_proxy const_word_range() const { return const_word_iterator_proxy(*this); }
	};
	
	
	template <unsigned int t_bits, typename t_word>
	std::ostream &operator<<(std::ostream &os, packed_vector <t_bits, t_word> const &vec)
	{
		ranges::copy(vec | ranges::view::transform([](auto val){ return +val; }), ranges::make_ostream_joiner(os, "\t"));
		return os;
	}
	
	
	template <unsigned int t_bits, typename t_word>
	auto packed_vector <t_bits, t_word>::load(
		std::size_t const idx,
		std::memory_order const order
	) const -> word_type
	{
		libbio_assert(idx < m_size);
		auto const word_idx(idx / ELEMENT_COUNT);
		auto const el_idx(idx % ELEMENT_COUNT);
		word_type const retval(m_values[word_idx].load(order));
		return ((retval >> (el_idx * t_bits)) & ELEMENT_MASK);
	}
	
	
	template <unsigned int t_bits, typename t_word>
	auto packed_vector <t_bits, t_word>::fetch_or(
		std::size_t const idx,
		word_type val,
		std::memory_order const order
	) -> word_type
	{
		// Determine the position of the given index
		// inside a word and shift the given value.
		libbio_assert(idx < m_size);
		libbio_assert_eq_msg(val, (val & ELEMENT_MASK), "Attempted to fetch_or '", +val, "'.");

		auto const word_idx(idx / ELEMENT_COUNT);
		auto const el_idx(idx % ELEMENT_COUNT);
		auto const shift_amt(t_bits * el_idx);
	
		val &= ELEMENT_MASK;
		val <<= shift_amt;
		
		word_type const retval(m_values[word_idx].fetch_or(val, order));
		return ((retval >> shift_amt) & ELEMENT_MASK);
	}
		
		
	template <unsigned int t_bits, typename t_word>
	auto packed_vector <t_bits, t_word>::fetch_and(
		std::size_t const idx,
		word_type val,
		std::memory_order const order
	) -> word_type
	{
		// Create a mask that has all bits set except for the given value.
		// Then use bitwise or with the given value and use fetch_and.
		libbio_assert(idx < m_size);
		libbio_assert(val == (val & ELEMENT_MASK));
		
		word_type mask(ELEMENT_MASK);
		auto const word_idx(idx / ELEMENT_COUNT);
		auto const el_idx(idx % ELEMENT_COUNT);
		auto const shift_amt(t_bits * el_idx);
		
		mask <<= shift_amt;
		val <<= shift_amt;
		mask = ~mask;
		val |= mask;
		
		word_type const retval(m_values[word_idx].fetch_and(val, order));
		return ((retval >> shift_amt) & ELEMENT_MASK);
	}
}

#endif

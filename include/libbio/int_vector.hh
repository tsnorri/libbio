/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_INT_VECTOR_HH
#define LIBBIO_INT_VECTOR_HH

#include <boost/iterator/iterator_facade.hpp>
#include <cmath>
#include <range/v3/algorithm/copy.hpp>
#include <range/v3/view/sliding.hpp>
#include <range/v3/view/transform.hpp>
#include <libbio/algorithm.hh>
#include <libbio/utility.hh>
#include <vector>


// FIXME: come up with better names. This could actually be packed_vector and the current packed_vector and packed_matrix something else to reflect the atomicity of their operations.
// FIXME: also consolidate the code where possible.

namespace libbio { namespace detail {
	
	template <unsigned int t_bits, typename t_word>
	struct int_vector_trait_base
	{
		constexpr inline t_word extent_mask(std::size_t const extent_size);
	};
	
	template <unsigned int t_bits, typename t_word>
	class int_vector_trait : public int_vector_trait_base <0, t_word>
	{
	public:
		typedef t_word		word_type;
		enum { WORD_BITS = CHAR_BIT * sizeof(word_type) };
		
		enum {
			ELEMENT_BITS		= t_bits,
			ELEMENT_COUNT		= WORD_BITS / t_bits,
			ELEMENT_MASK		= (std::numeric_limits <word_type>::max() >> (WORD_BITS - t_bits))
		};
		
		int_vector_trait() = default;
		
		int_vector_trait(std::uint8_t /* ignored */):
			int_vector_trait()
		{
		}
		
		constexpr std::uint8_t element_bits() const { return ELEMENT_BITS; }
		constexpr std::uint8_t element_count_in_word() const { return ELEMENT_COUNT; }
		constexpr std::uint8_t bits_before_element(std::uint8_t const el_idx) const { return t_bits * el_idx; }
		constexpr word_type element_mask() const { return ELEMENT_MASK; }
		constexpr inline word_type extent_mask(std::size_t const extent_size, word_type const mask);
	};
	
	// Specialization for runtime-defined element size.
	template <typename t_word>
	class int_vector_trait <0, t_word> : public int_vector_trait_base <0, t_word>
	{
	public:
		typedef t_word		word_type;
		enum { WORD_BITS = CHAR_BIT * sizeof(word_type) };
		
	protected:
		std::uint8_t	m_bits{};
		
	public:
		int_vector_trait() = default;
		
		int_vector_trait(std::uint8_t bits):
			m_bits(bits)
		{
		}
		
		std::uint8_t element_bits() const { return m_bits; }
		void set_element_bits(std::uint8_t const bits) { m_bits = bits; }
		
		std::uint8_t element_count_in_word() const { return WORD_BITS / m_bits; }
		std::uint8_t bits_before_element(std::uint8_t const el_idx) const { return m_bits * el_idx; }
		word_type element_mask() const { return (std::numeric_limits <word_type>::max() >> (WORD_BITS - m_bits)); }
		inline word_type extent_mask(std::size_t const extent_size, word_type const mask);
	};
	
	
	template <typename t_vector>
	class int_vector_value_reference
	{
	public:
		typedef typename t_vector::word_type	word_type;
		
	protected:
		t_vector			*m_vector{};
		std::size_t			m_idx{};
		
	public:
		int_vector_value_reference() = default;
		
		int_vector_value_reference(t_vector &vector, std::size_t idx):
			m_vector(&vector),
			m_idx(idx)
		{
		}
		
	public:
		word_type load() const { return m_vector->load(m_idx); }
		void assign_or(word_type val) { m_vector->assign_or(m_idx, val); }
		
		operator word_type() const { return load(); }
		int_vector_value_reference &operator|=(word_type val) { assign_or(val); return *this; }
	};
	
	
	template <typename t_vector, bool t_is_const = std::is_const_v <t_vector>>
	struct int_vector_iterator_traits
	{
		typedef typename t_vector::reference			reference;
		typedef typename t_vector::iterator				iterator;
		typedef typename t_vector::word_iterator		word_iterator;
	};
	
	template <typename t_vector>
	struct int_vector_iterator_traits <t_vector, true>
	{
		typedef typename t_vector::const_reference		reference;
		typedef typename t_vector::const_iterator		iterator;
		typedef typename t_vector::const_word_iterator	word_iterator;
	};
	
	
	template <typename t_vector>
	class int_vector_iterator_base
	{
	public:
		typedef int_vector_iterator_traits <t_vector>	traits;
		typedef typename traits::word_iterator			word_iterator;
		typedef typename t_vector::reference_proxy		reference_proxy;	// FIXME: needed?
		
		enum { ELEMENT_COUNT = t_vector::ELEMENT_COUNT };

	protected:
		t_vector		*m_vector{};
		std::size_t		m_idx{};
		
	public:
		virtual ~int_vector_iterator_base() {}

		int_vector_iterator_base() = default;
		int_vector_iterator_base(t_vector &vector, std::size_t idx):
			m_vector(&vector),
			m_idx(idx)
		{
		}
		
		template <
			typename t_other_vector,
			typename std::enable_if_t <std::is_convertible_v <t_other_vector *, t_vector *>>
		> int_vector_iterator_base(int_vector_iterator_base <t_other_vector> const &other):
			m_vector(other.m_vector),
			m_idx(other.m_idx)
		{
		}
		
		std::size_t index() const { return m_idx; }
		std::size_t word_index() const { return m_idx / ELEMENT_COUNT; }
		std::size_t word_offset() const { return m_idx % ELEMENT_COUNT; }
		
		virtual void advance(std::ptrdiff_t) = 0;
		bool equal(int_vector_iterator_base const &other) const { return m_vector == other.m_vector && m_idx == other.m_idx; }
		
		void increment() { advance(1); }
		void decrement() { advance(-1); }
		typename traits::reference dereference() const { return m_vector->operator()(m_idx); }
		
		reference_proxy to_reference_proxy() const { return reference_proxy(*m_vector, m_idx); }
		word_iterator to_containing_word_iterator() const { return m_vector->word_begin() + m_idx / ELEMENT_COUNT; }
		word_iterator to_word_iterator() const
		{
			if (0 != word_offset())
				throw std::runtime_error("Unable to convert to word_iterator");
			return m_vector->word_begin() + m_idx / ELEMENT_COUNT;
		}
		
		operator reference_proxy() const { return to_reference_proxy(); }

		std::ptrdiff_t distance_to(int_vector_iterator_base const &other) const
		{
			libbio_assert(this->m_idx <= std::numeric_limits <std::ptrdiff_t>::max());
			libbio_assert(other.m_idx <= std::numeric_limits <std::ptrdiff_t>::max());
			auto const retval(other.m_idx - this->m_idx);
			libbio_assert_lte(std::numeric_limits <std::ptrdiff_t>::min(), retval);
			return retval;
		}
	};
	
	
	template <typename t_vector>
	class int_vector_iterator final :
		public int_vector_iterator_base <t_vector>,
		public boost::iterator_facade <
			int_vector_iterator <t_vector>,
			typename int_vector_iterator_traits <t_vector>::reference,
			boost::random_access_traversal_tag,
			typename int_vector_iterator_traits <t_vector>::reference
		>
	{
		friend class boost::iterator_core_access;
		
	protected:
		typedef int_vector_iterator_base <t_vector>	iterator_base;
		
	public:
		using iterator_base::int_vector_iterator_base;
		
	private:
		virtual void advance(std::ptrdiff_t const diff) override { this->m_idx += diff; }
		std::ptrdiff_t distance_to(int_vector_iterator const &other) const { return iterator_base::distance_to(other); }
	};
}}


namespace libbio {
	
	template <unsigned int t_bits, typename t_word = std::uint64_t>
	class int_vector : public detail::int_vector_trait <t_bits, t_word>
	{
		static_assert(std::is_unsigned_v <t_word>);
		
	public:
		typedef t_word		word_type;
		typedef detail::int_vector_trait <t_bits, t_word>				trait_type;
		enum { WORD_BITS = trait_type::WORD_BITS };
		
		static_assert(t_bits <= WORD_BITS);
		static_assert(0 == t_bits || 0 == WORD_BITS % t_bits);
		
	protected:
		typedef std::vector <word_type>									value_vector;
		
		struct protected_tag {};
		
	public:
		typedef typename value_vector::reference						word_reference;
		typedef typename value_vector::iterator							word_iterator;
		typedef typename value_vector::const_iterator					const_word_iterator;
		typedef typename value_vector::reverse_iterator					reverse_word_iterator;
		typedef typename value_vector::const_reverse_iterator			const_reverse_word_iterator;
		
		typedef detail::int_vector_value_reference <int_vector>			reference_proxy;
		typedef reference_proxy											reference;
		typedef word_type												const_reference;
		
		typedef detail::int_vector_iterator <int_vector>				iterator;
		typedef detail::int_vector_iterator <int_vector const>			const_iterator;
		
	protected:
		value_vector	m_values;
		std::size_t		m_size{};
		
	protected:
		int_vector(std::size_t const size, std::uint8_t const bits, protected_tag const &):
			trait_type(bits),
			m_values(std::ceil(1.0 * size / this->element_count_in_word()), 0),
			m_size(size)
		{
		}
		
	public:
		int_vector() = default;
		
		template <bool t_dummy = true, std::enable_if_t <t_dummy && 0 != t_bits, int> = 0>
		explicit int_vector(std::size_t const size):
			int_vector(size, 0, protected_tag())
		{
		}
		
		template <bool t_dummy = true, std::enable_if_t <t_dummy && 0 == t_bits, int> = 0>
		int_vector(std::size_t const size, std::uint8_t const bits):
			int_vector(size, bits, protected_tag())
		{
		}
		
		template <bool t_dummy = true, std::enable_if_t <t_dummy && 0 != t_bits, int> = 0>
		int_vector(std::size_t const size, word_type const val):
			trait_type(),
			m_values(std::ceil(1.0 * size / trait_type::element_count_in_word()), fill_bit_pattern <trait_type::ELEMENT_BITS>(val)),
			m_size(size)
		{
			auto const extent_size(size % this->element_count_in_word());
			if (extent_size)
			{
				auto const last_element_mask(this->extent_mask(extent_size, val));
				*m_values.rbegin() = last_element_mask;
			}
		}
		
		// Primitives.
		word_type load(std::size_t const idx) const;
		void assign_or(std::size_t const idx, word_type val);
		void assign_and(std::size_t const idx, word_type val);
		
		word_type operator()(std::size_t const idx) const { return load(idx); }
		reference_proxy operator()(std::size_t const idx) { return reference_proxy(*this, idx); }
		word_type word_at(std::size_t const idx) const { return m_values[idx]; }
		word_reference word_at(std::size_t const idx) { return m_values[idx]; }
		
		// Since memory order is not needed here, operator[] can also be defined.
		word_type operator[](std::size_t const idx) const { return load(idx); }
		reference_proxy operator[](std::size_t const idx) { return reference_proxy(*this, idx); }
		
		// Convenience functions.
		word_type front() const { return (*this)[0]; }
		reference_proxy front() { return (*this)[0]; }
		word_type back() const { return (*this)[size() - 1]; }
		reference_proxy back() { return (*this)[size() - 1]; }
		
		std::size_t size() const { return m_size; }											// Size in elements.
		std::size_t reserved_size() const { return m_values.size() * this->element_count_in_word(); } // FIXME: not completely true b.c. m_values may have more space allocated than its size().
		std::size_t word_size() const { return m_values.size(); }
		void set_size(std::size_t new_size) { libbio_assert(new_size <= reserved_size()); m_size = new_size; }
		
		// Since m_values does not contain std::atomic <...>, its contents can be moved and thus the vector can be resized.
		void resize(std::size_t new_size) { m_values.resize(std::ceil(1.0 * new_size / this->element_count_in_word())); m_size = new_size; }
		void resize(std::size_t new_size, word_type bit_pattern) { m_values.resize(std::ceil(1.0 * new_size / this->element_count_in_word())); m_size = new_size; }
		void reserve(std::size_t new_size) { m_values.reserve(std::ceil(1.0 * new_size / this->element_count_in_word())); }
		
		constexpr std::size_t word_bits() const { return WORD_BITS; }
		
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
		reverse_word_iterator word_rbegin() { return m_values.rbegin(); }
		reverse_word_iterator word_rend() { return m_values.rend(); }
		const_reverse_word_iterator word_rbegin() const { return m_values.rbegin(); }
		const_reverse_word_iterator word_rend() const { return m_values.rend(); }
		const_reverse_word_iterator word_crbegin() const { return m_values.crbegin(); }
		const_reverse_word_iterator word_crend() const { return m_values.crend(); }
		
		// Additional helpers.
		template <bool t_dummy = true, std::enable_if_t <t_dummy && 0 == t_bits, int> = 0>
		void push_back(word_type val);
		
		template <bool t_dummy = true, std::enable_if_t <t_dummy && 0 != t_bits, int> = 0>
		void push_back(word_type mask, std::size_t count = 1);
		
		template <bool t_dummy = true, std::enable_if_t <t_dummy && 0 != t_bits, int> = 0>
		void reverse();
		
		void clear() { m_values.clear(); m_size = 0; }
		
		bool operator==(int_vector const &other) const;
	};
	
	
	typedef int_vector <1> bit_vector;
}


namespace libbio { namespace detail {
	
	template <unsigned int t_bits, typename t_word>
	constexpr auto int_vector_trait_base <t_bits, t_word>::extent_mask(std::size_t const size) -> t_word
	{
		libbio_assert(size <= this->element_count_in_word());
		t_word const mask(~t_word(0));
		return mask >> ((this->element_count_in_word() - size) * this->element_bits());
	}
	
	
	template <unsigned int t_bits, typename t_word>
	constexpr auto int_vector_trait <t_bits, t_word>::extent_mask(std::size_t const size, word_type const mask) -> word_type
	{
		libbio_assert(size <= this->element_count_in_word());
		return fill_bit_pattern <ELEMENT_BITS>(mask) >> ((this->element_count_in_word() - size) * this->element_bits());
	}
	
	
	template <typename t_word>
	auto int_vector_trait <0, t_word>::extent_mask(std::size_t const size, word_type const mask) -> word_type
	{
		libbio_assert(size <= this->element_count_in_word());
		return fill_bit_pattern(mask, this->element_bits()) >> ((this->element_count_in_word() - size) * this->element_bits());
	}
}}


namespace libbio {
	
	template <unsigned int t_bits, typename t_word>
	std::ostream &operator<<(std::ostream &os, int_vector <t_bits, t_word> const &vec)
	{
		ranges::copy(vec | ranges::view::transform([](auto val){ return +val; }), ranges::make_ostream_joiner(os, "\t"));
		return os;
	}
	
	
	template <unsigned int t_bits, typename t_word>
	auto int_vector <t_bits, t_word>::load(
		std::size_t const idx
	) const -> word_type
	{
		libbio_assert(idx < m_size);
		auto const word_idx(idx / this->element_count_in_word());
		auto const el_idx(idx % this->element_count_in_word());
		word_type const retval(m_values[word_idx]);
		return ((retval >> (el_idx * t_bits)) & this->element_mask());
	}
	
	
	template <unsigned int t_bits, typename t_word>
	void int_vector <t_bits, t_word>::assign_or(
		std::size_t const idx,
		word_type val
	)
	{
		// Determine the position of the given index
		// inside a word and shift the given value.
		libbio_assert(idx < m_size);
		libbio_assert(val == (val & this->element_mask()));

		auto const word_idx(idx / this->element_count_in_word());
		auto const el_idx(idx % this->element_count_in_word());
		auto const shift_amt(t_bits * el_idx);
	
		val &= this->element_mask();
		val <<= shift_amt;
		
		m_values[word_idx] |= val;
	}
		
		
	template <unsigned int t_bits, typename t_word>
	void int_vector <t_bits, t_word>::assign_and(
		std::size_t const idx,
		word_type val
	)
	{
		// Create a mask that has all bits set except for the given value.
		// Then use bitwise or with the given value and use fetch_and.
		libbio_assert(idx < m_size);
		libbio_assert(val == (val & this->element_mask()));
		
		word_type mask(this->element_mask());
		auto const word_idx(idx / this->element_count_in_word());
		auto const el_idx(idx % this->element_count_in_word());
		auto const shift_amt(t_bits * el_idx);
		
		mask <<= shift_amt;
		val <<= shift_amt;
		mask = ~mask;
		val |= mask;
		
		m_values[word_idx] &= val;
	}
	
	
	template <unsigned int t_bits, typename t_word>
	template <bool t_dummy, std::enable_if_t <t_dummy && 0 == t_bits, int>>
	void int_vector <t_bits, t_word>::push_back(word_type val)
	{
		val &= this->element_mask();
		
		if (size() == reserved_size())
			m_values.push_back(val);
		else
			assign_or(this->size() - 1, val);
	}
	
	
	template <unsigned int t_bits, typename t_word>
	template <bool t_dummy, std::enable_if_t <t_dummy && 0 != t_bits, int>>
	void int_vector <t_bits, t_word>::push_back(word_type val, std::size_t count)
	{
		if (0 == count)
			return;
		
		// Create a mask that repeats the given value.
		val &= this->element_mask();
		word_type const mask(fill_bit_pattern <trait_type::ELEMENT_BITS>(val));
		
		// If m_size is not a multiple element_count_in_word(), there’s space in the final word.
		if (m_size % this->element_count_in_word())
		{
			auto const last_word_elements(m_size % this->element_count_in_word());
			auto const remaining_space_elements(this->element_count_in_word() - last_word_elements);
			word_type new_mask(mask);
			
			// Erase elements if needed.
			std::size_t inserted_count(remaining_space_elements);
			if (count < remaining_space_elements)
			{
				inserted_count = count;
				new_mask >>= (this->element_count_in_word() - count) * this->element_bits();
			}
			
			new_mask <<= last_word_elements * this->element_bits();
			*m_values.rbegin() |= new_mask;
			
			m_size += inserted_count;
			count -= inserted_count;
		}
		
		while (this->element_count_in_word() < count)
		{
			// Check that m_size is t_word aligned in order to append a word at a time.
			libbio_assert(0 == m_size % this->element_count_in_word());
			
			m_values.push_back(mask);
			count -= this->element_count_in_word();
			m_size += this->element_count_in_word();
		}
		
		// Append the extent if needed.
		if (count)
		{
			// Erase elements from the mask.
			word_type const new_mask(mask >> ((this->element_count_in_word() - count) * this->element_bits()));
			m_values.push_back(new_mask);
			m_size += count;
		}
	}
	
	
	template <unsigned int t_bits, typename t_word>
	template <bool t_dummy, std::enable_if_t <t_dummy && 0 != t_bits, int>>
	void int_vector <t_bits, t_word>::reverse()
	{
		if (0 == m_values.size())
			return;
		
		// Reverse the word order.
		std::reverse(m_values.begin(), m_values.end());
		
		for (auto &val : m_values)
			val = reverse_bits <trait_type::ELEMENT_BITS>(val);
		
		auto const shift_left((m_size % this->element_count_in_word()) * this->element_bits());
		auto const shift_right(WORD_BITS - shift_left);
		for (auto const &pair : m_values | ranges::view::sliding(2))
		{
			pair[0] >>= shift_right;
			pair[0] |= (pair[1] << shift_left);
		}
		*m_values.rbegin() >>= shift_right;
	}
	
	
	template <unsigned int t_bits, typename t_word>
	bool int_vector <t_bits, t_word>::operator==(int_vector const &other) const
	{
		if (m_size != other.m_size)
			return false;
		
		auto const extent_size(m_size % this->element_count_in_word());
		if (0 == extent_size)
			return m_values == other.m_values;
		
		libbio_assert(m_values.size());
		libbio_assert(other.m_values.size());
		
		if (!std::equal(m_values.cbegin(), m_values.cend() - 1, other.m_values.cbegin(), other.m_values.cend() - 1))
			return false;
		
		auto const mask(this->extent_mask(extent_size));
		return (*m_values.crbegin() & mask) == (*other.m_values.crbegin() & mask);
	}
}

#endif

/*
 * Copyright (c) 2018–2019 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_INT_VECTOR_INT_VECTOR_HH
#define LIBBIO_INT_VECTOR_INT_VECTOR_HH

#include <boost/iterator/iterator_facade.hpp>
#include <cmath>
#include <range/v3/algorithm/copy.hpp>
#include <range/v3/view/sliding.hpp>
#include <range/v3/view/transform.hpp>
#include <libbio/algorithm.hh>
#include <libbio/int_vector/iterator.hh>
#include <libbio/int_vector/value_reference.hh>
#include <libbio/int_vector/width.hh>
#include <libbio/int_vector/word_iterator_proxy.hh>
#include <libbio/utility.hh>
#include <vector>


namespace libbio {
	template <unsigned int, typename, template <typename, unsigned int, typename> typename>
	class int_vector_tpl;
}


namespace libbio { namespace detail {
	
	// Traits / mixins.
	
	struct int_vector_trait_base
	{
	protected:
		std::size_t calculate_word_count(std::size_t const elements, std::size_t const element_count_in_word) const { return std::ceil(1.0 * elements / element_count_in_word); }
	};
	
	
	template <typename t_vector, unsigned int t_bits, typename t_word>
	struct int_vector_trait : public int_vector_trait_base
	{
		typedef detail::int_vector_width <t_bits, t_word>		width_type;
		typedef t_word											word_type;
		typedef std::vector <word_type>							value_vector;
		typedef typename value_vector::reference				word_reference;
		typedef detail::int_vector_value_reference <t_vector>	reference_proxy;
		typedef reference_proxy									reference;
		typedef word_type										const_reference;
		
		enum {
			WORD_BITS		= width_type::WORD_BITS,
			ELEMENT_BITS	= width_type::ELEMENT_BITS
		};
		
		// Primitives.
		inline word_type load(std::size_t const idx) const;
		inline void assign_or(std::size_t const idx, word_type val);
		inline void assign_and(std::size_t const idx, word_type val);
		
		word_type operator()(std::size_t const idx) const { return load(idx); }
		reference_proxy operator()(std::size_t const idx) { return reference_proxy(as_vector(), idx); }
		word_type word_at(std::size_t const idx) const { return as_vector().m_values[idx]; }
		word_reference word_at(std::size_t const idx) { return as_vector().m_values[idx]; }
		
		// Since memory order is not needed here, operator[] can also be defined.
		word_type operator[](std::size_t const idx) const { return load(idx); }
		reference_proxy operator[](std::size_t const idx) { return reference_proxy(as_vector(), idx); }
		
		// Convenience functions.
		word_type front() const { return (*this)[0]; }
		reference_proxy front() { return (*this)[0]; }
		word_type back() const { return (*this)[as_vector().size() - 1]; }
		reference_proxy back() { return (*this)[as_vector().size() - 1]; }
		
		// Resizing
		// Since m_values does not contain std::atomic <...>, its contents can be moved and thus the vector can be resized.
		void resize(std::size_t new_size);
		void resize(std::size_t new_size, word_type bit_pattern);
		void reserve(std::size_t new_size);
		
		// Additional helpers.
		template <bool t_dummy = true, std::enable_if_t <t_dummy && 0 == ELEMENT_BITS, int> = 0>
		void push_back(word_type val);
		
		template <bool t_dummy = true, std::enable_if_t <t_dummy && 0 != ELEMENT_BITS, int> = 0>
		void push_back(word_type mask, std::size_t count = 1);
		
		template <bool t_dummy = true, std::enable_if_t <t_dummy && 0 != ELEMENT_BITS, int> = 0>
		void reverse();
		
		void clear();
		
	protected:
		value_vector make_value_vector(std::size_t const size) const { return value_vector(calculate_word_count(size, as_vector().element_count_in_word()), 0); }
		t_vector &as_vector() { return static_cast <t_vector &>(*this); }
		t_vector const &as_vector() const { return static_cast <t_vector const &>(*this); }
	};
	
	
	template <typename t_vector, unsigned int t_bits, typename t_word>
	struct atomic_int_vector_trait : public int_vector_trait_base
	{
		typedef detail::int_vector_width <t_bits, t_word>				width_type;
		typedef t_word													word_type;
		typedef std::vector <std::atomic <word_type>>					value_vector;
		typedef typename value_vector::reference						word_reference;
		typedef detail::atomic_int_vector_value_reference <t_vector>	reference_proxy;
		typedef reference_proxy											reference;
		typedef t_word													const_reference;
		
		enum {
			WORD_BITS		= width_type::WORD_BITS,
			ELEMENT_BITS	= width_type::ELEMENT_BITS
		};
		
		// Primitives.
		inline word_type load(std::size_t const idx, std::memory_order = std::memory_order_seq_cst) const;
		inline word_type fetch_or(std::size_t const idx, word_type val, std::memory_order const = std::memory_order_seq_cst);
		inline word_type fetch_and(std::size_t const idx, word_type val, std::memory_order const = std::memory_order_seq_cst);
		
		word_type operator()(std::size_t const idx, std::memory_order order = std::memory_order_seq_cst) const { return load(idx, order); }
		reference_proxy operator()(std::size_t const idx) { return reference_proxy(as_vector(), idx); }
		word_type word_at(std::size_t const idx) const { return as_vector().m_values[idx]; }
		word_reference word_at(std::size_t const idx) { return as_vector().m_values[idx]; }
		
		// Convenience functions.
		// FIXME: consider adding a memory order to the non-const versions of the functions below and adding it as a variable to the proxy.
		// This will cause the reference’s load() etc. to work slightly different from the one defined in this struct above.
		word_type front(std::memory_order order = std::memory_order_seq_cst) const { return (*this)(0, order); }
		reference_proxy front() { return (*this)[0]; }
		word_type back(std::memory_order order = std::memory_order_seq_cst) const { return (*this)(as_vector().size() - 1, order); }
		reference_proxy back() { return (*this)[as_vector().size() - 1]; }
		
	protected:
		value_vector make_value_vector(std::size_t const size) const { return value_vector(calculate_word_count(size, as_vector().element_count_in_word())); }
		t_vector &as_vector() { return static_cast <t_vector &>(*this); }
		t_vector const &as_vector() const { return static_cast <t_vector const &>(*this); }
	};
	
	
	template <unsigned int t_bits, typename t_word, template <typename, unsigned int, typename> typename t_trait>
	using int_vector_base = t_trait <int_vector_tpl <t_bits, t_word, t_trait>, t_bits, t_word>;
}}


namespace libbio {
	
	template <unsigned int t_bits, typename t_word, template <typename, unsigned int, typename> typename t_trait>
	class int_vector_tpl final :
		public detail::int_vector_width <t_bits, t_word>,
		public detail::int_vector_base <t_bits, t_word, t_trait>
	{
		static_assert(std::is_unsigned_v <t_word>);
		
		template <typename t_archive, unsigned int t_bits_1, typename t_word_1, template <typename, unsigned int, typename> typename t_trait_1>
		friend void serialize(t_archive &, int_vector_tpl <t_bits_1, t_word_1, t_trait_1> &);
		
	public:
		typedef t_word																	word_type;
		typedef detail::int_vector_width <t_bits, t_word>								width_type;
		typedef detail::int_vector_base <t_bits, t_word, t_trait>						trait_type;
		
		enum {
			WORD_BITS		= width_type::WORD_BITS,
			ELEMENT_BITS	= width_type::ELEMENT_BITS
		};
		
		friend trait_type;
		
		static_assert(t_bits <= WORD_BITS);
		static_assert(0 == t_bits || 0 == WORD_BITS % t_bits);
		
	protected:
		typedef typename trait_type::value_vector										value_vector;
		
		struct protected_tag {};
		
	public:
		typedef typename value_vector::reference										word_reference;
		typedef typename value_vector::iterator											word_iterator;
		typedef typename value_vector::const_iterator									const_word_iterator;
		typedef typename value_vector::reverse_iterator									reverse_word_iterator;
		typedef typename value_vector::const_reverse_iterator							const_reverse_word_iterator;
		typedef detail::int_vector_word_iterator_proxy <int_vector_tpl>					word_iterator_proxy;
		typedef detail::int_vector_word_iterator_proxy <int_vector_tpl const>			const_word_iterator_proxy;
		typedef detail::int_vector_reverse_word_iterator_proxy <int_vector_tpl>			reverse_word_iterator_proxy;
		typedef detail::int_vector_reverse_word_iterator_proxy <int_vector_tpl const>	const_reverse_word_iterator_proxy;
		
		typedef typename trait_type::reference											reference;
		typedef typename trait_type::const_reference									const_reference;
		
		typedef detail::int_vector_iterator <int_vector_tpl>							iterator;
		typedef detail::int_vector_iterator <int_vector_tpl const>						const_iterator;
		
	protected:
		value_vector	m_values;
		std::size_t		m_size{};
		
	protected:
		int_vector_tpl(std::size_t const size, std::uint8_t const bits, protected_tag const &):
			width_type(bits),
			m_values(this->make_value_vector(size)),
			m_size(size)
		{
		}
		
	public:
		int_vector_tpl() = default;
		
		template <bool t_dummy = true, std::enable_if_t <t_dummy && 0 != t_bits, int> = 0>
		explicit int_vector_tpl(std::size_t const size):
			int_vector_tpl(size, 0, protected_tag())
		{
		}
		
		template <bool t_dummy = true, std::enable_if_t <t_dummy && 0 == t_bits, int> = 0>
		int_vector_tpl(std::size_t const size, std::uint8_t const bits):
			int_vector_tpl(size, bits, protected_tag())
		{
		}
		
		template <bool t_dummy = true, std::enable_if_t <t_dummy && 0 != t_bits, int> = 0>
		int_vector_tpl(std::size_t const size, word_type const val):
			width_type(),
			m_values(std::ceil(1.0 * size / width_type::element_count_in_word()), fill_bit_pattern <width_type::ELEMENT_BITS>(val)),
			m_size(size)
		{
			auto const extent_size(size % this->element_count_in_word());
			if (extent_size)
			{
				auto const last_element_mask(this->extent_mask(extent_size, val));
				*m_values.rbegin() = last_element_mask;
			}
		}
		
		bool operator==(int_vector_tpl const &other) const;
		
		// Size
		std::size_t size() const { return m_size; }											// Size in elements.
		std::size_t reserved_size() const { return m_values.size() * this->element_count_in_word(); } // FIXME: not completely true b.c. m_values may have more space allocated than its size().
		std::size_t word_size() const { return m_values.size(); }
		void set_size(std::size_t new_size) { libbio_assert(new_size <= reserved_size()); m_size = new_size; }
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
		word_iterator_proxy word_range() { return word_iterator_proxy(*this); }
		const_word_iterator_proxy word_range() const { return const_word_iterator_proxy(*this); }
		const_word_iterator_proxy const_word_range() const { return const_word_iterator_proxy(*this); }
		reverse_word_iterator_proxy reverse_word_range() { return reverse_word_iterator_proxy(*this); }
		const_reverse_word_iterator_proxy reverse_word_range() const { return const_reverse_word_iterator_proxy(*this); }
		const_reverse_word_iterator_proxy reverse_const_word_range() const { return const_reverse_word_iterator_proxy(*this); }
	};
	
	
	template <unsigned int t_bits, typename t_word = std::uint64_t>
	using int_vector = int_vector_tpl <t_bits, t_word, detail::int_vector_trait>;
	
	template <unsigned int t_bits, typename t_word = std::uint64_t>
	using atomic_int_vector = int_vector_tpl <t_bits, t_word, detail::atomic_int_vector_trait>;
	
	typedef int_vector <1> bit_vector;
	typedef atomic_int_vector <1> atomic_bit_vector;
}


namespace libbio { namespace detail {
	
	template <typename t_vector, unsigned int t_bits, typename t_word>
	auto int_vector_trait <t_vector, t_bits, t_word>::load(std::size_t const idx) const -> word_type
	{
		auto &self(as_vector());
		libbio_assert(idx < self.m_size);
		auto const word_idx(idx / self.element_count_in_word());
		auto const el_idx(idx % self.element_count_in_word());
		word_type const retval(self.m_values[word_idx]);
		return ((retval >> (el_idx * self.element_bits())) & self.element_mask());
	}
	
	
	template <typename t_vector, unsigned int t_bits, typename t_word>
	void int_vector_trait <t_vector, t_bits, t_word>::assign_or(
		std::size_t const idx,
		word_type val
	)
	{
		// Determine the position of the given index
		// inside a word and shift the given value.
		auto &self(as_vector());
		libbio_assert(idx < self.m_size);
		libbio_assert(val == (val & self.element_mask()));

		auto const word_idx(idx / self.element_count_in_word());
		auto const el_idx(idx % self.element_count_in_word());
		auto const shift_amt(self.element_bits() * el_idx);
	
		val &= self.element_mask();
		val <<= shift_amt;
		
		self.m_values[word_idx] |= val;
	}
		
		
	template <typename t_vector, unsigned int t_bits, typename t_word>
	void int_vector_trait <t_vector, t_bits, t_word>::assign_and(
		std::size_t const idx,
		word_type val
	)
	{
		// Create a mask that has all bits set except for the given value.
		// Then use bitwise or with the given value and use fetch_and.
		auto &self(as_vector());
		libbio_assert(idx < self.m_size);
		libbio_assert(val == (val & self.element_mask()));
		
		word_type mask(self.element_mask());
		auto const word_idx(idx / self.element_count_in_word());
		auto const el_idx(idx % self.element_count_in_word());
		auto const shift_amt(self.element_bits() * el_idx);
		
		mask <<= shift_amt;
		val <<= shift_amt;
		mask = ~mask;
		val |= mask;
		
		self.m_values[word_idx] &= val;
	}
	
	
	template <typename t_vector, unsigned int t_bits, typename t_word>
	void int_vector_trait <t_vector, t_bits, t_word>::resize(std::size_t new_size)
	{
		auto &self(as_vector());
		self.m_values.resize(std::ceil(1.0 * new_size / self.element_count_in_word()));
		self.m_size = new_size;
	}
	
	
	template <typename t_vector, unsigned int t_bits, typename t_word>
	void int_vector_trait <t_vector, t_bits, t_word>::resize(std::size_t new_size, word_type bit_pattern)
	{
		auto &self(as_vector());
		self.m_values.resize(std::ceil(1.0 * new_size / self.element_count_in_word()), bit_pattern);
		self.m_size = new_size;
	}
	
	
	template <typename t_vector, unsigned int t_bits, typename t_word>
	void int_vector_trait <t_vector, t_bits, t_word>::reserve(std::size_t new_size)
	{
		auto &self(as_vector());
		self.m_values.reserve(std::ceil(1.0 * new_size / self.element_count_in_word()));
	}
	
	
	template <typename t_vector, unsigned int t_bits, typename t_word>
	void int_vector_trait <t_vector, t_bits, t_word>::clear()
	{
		auto &self(as_vector());
		self.m_values.clear();
		self.m_size = 0;
	}
	
	
	template <typename t_vector, unsigned int t_bits, typename t_word>
	template <bool t_dummy, std::enable_if_t <t_dummy && 0 == int_vector_trait <t_vector, t_bits, t_word>::ELEMENT_BITS, int>>
	void int_vector_trait <t_vector, t_bits, t_word>::push_back(word_type val)
	{
		auto &self(as_vector());
		val &= self.element_mask();
		
		if (self.size() == self.reserved_size())
			self.m_values.push_back(val);
		else
			assign_or(self.size() - 1, val);
	}
	
	
	template <typename t_vector, unsigned int t_bits, typename t_word>
	template <bool t_dummy, std::enable_if_t <t_dummy && 0 != int_vector_trait <t_vector, t_bits, t_word>::ELEMENT_BITS, int>>
	void int_vector_trait <t_vector, t_bits, t_word>::push_back(word_type val, std::size_t count)
	{
		if (0 == count)
			return;
		
		// Create a mask that repeats the given value.
		auto &self(as_vector());
		val &= self.element_mask();
		word_type const mask(fill_bit_pattern <t_vector::ELEMENT_BITS>(val));
		
		// If m_size is not a multiple element_count_in_word(), there’s space in the final word.
		if (self.m_size % self.element_count_in_word())
		{
			auto const last_word_elements(self.m_size % self.element_count_in_word());
			auto const remaining_space_elements(self.element_count_in_word() - last_word_elements);
			word_type new_mask(mask);
			
			// Erase elements if needed.
			std::size_t inserted_count(remaining_space_elements);
			if (count < remaining_space_elements)
			{
				inserted_count = count;
				new_mask >>= (self.element_count_in_word() - count) * self.element_bits();
			}
			
			new_mask <<= last_word_elements * self.element_bits();
			*self.m_values.rbegin() |= new_mask;
			
			self.m_size += inserted_count;
			count -= inserted_count;
		}
		
		while (self.element_count_in_word() < count)
		{
			// Check that m_size is t_word aligned in order to append a word at a time.
			libbio_assert(0 == self.m_size % self.element_count_in_word());
			
			self.m_values.push_back(mask);
			count -= self.element_count_in_word();
			self.m_size += self.element_count_in_word();
		}
		
		// Append the extent if needed.
		if (count)
		{
			// Erase elements from the mask.
			word_type const new_mask(mask >> ((self.element_count_in_word() - count) * self.element_bits()));
			self.m_values.push_back(new_mask);
			self.m_size += count;
		}
	}
	
	
	template <typename t_vector, unsigned int t_bits, typename t_word>
	template <bool t_dummy, std::enable_if_t <t_dummy && 0 != int_vector_trait <t_vector, t_bits, t_word>::ELEMENT_BITS, int>>
	void int_vector_trait <t_vector, t_bits, t_word>::reverse()
	{
		auto &self(as_vector());
		if (0 == self.m_values.size())
			return;
		
		// Reverse the word order.
		std::reverse(self.m_values.begin(), self.m_values.end());
		
		for (auto &val : self.m_values)
			val = reverse_bits <ELEMENT_BITS>(val);
		
		auto const shift_left((self.m_size % self.element_count_in_word()) * self.element_bits());
		auto const shift_right(WORD_BITS - shift_left);
		for (auto const &pair : self.m_values | ranges::view::sliding(2))
		{
			pair[0] >>= shift_right;
			pair[0] |= (pair[1] << shift_left);
		}
		*self.m_values.rbegin() >>= shift_right;
	}
	
	
	template <typename t_vector, unsigned int t_bits, typename t_word>
	auto atomic_int_vector_trait <t_vector, t_bits, t_word>::load(
		std::size_t const idx,
		std::memory_order const order
	) const -> word_type
	{
		auto &self(as_vector());
		libbio_assert(idx < self.m_size);
		auto const word_idx(idx / self.element_count_in_word());
		auto const el_idx(idx % self.element_count_in_word());
		word_type const retval(self.m_values[word_idx].load(order));
		return ((retval >> self.bits_before_element(el_idx)) & self.element_mask());
	}
	
	
	template <typename t_vector, unsigned int t_bits, typename t_word>
	auto atomic_int_vector_trait <t_vector, t_bits, t_word>::fetch_or(
		std::size_t const idx,
		word_type val,
		std::memory_order const order
	) -> word_type
	{
		// Determine the position of the given index
		// inside a word and shift the given value.
		auto &self(as_vector());
		libbio_assert(idx < self.m_size);
		libbio_assert_eq_msg(val, (val & self.element_mask()), "Attempted to fetch_or '", +val, "'.");

		auto const word_idx(idx / self.element_count_in_word());
		auto const el_idx(idx % self.element_count_in_word());
		auto const shift_amt(self.bits_before_element(el_idx));
	
		val &= self.element_mask();
		val <<= shift_amt;
		
		word_type const retval(self.m_values[word_idx].fetch_or(val, order));
		return ((retval >> shift_amt) & self.element_mask());
	}
		
		
	template <typename t_vector, unsigned int t_bits, typename t_word>
	auto atomic_int_vector_trait <t_vector, t_bits, t_word>::fetch_and(
		std::size_t const idx,
		word_type val,
		std::memory_order const order
	) -> word_type
	{
		// Create a mask that has all bits set except for the given value.
		// Then use bitwise or with the given value and use fetch_and.
		auto &self(as_vector());
		libbio_assert(idx < self.m_size);
		libbio_assert(val == (val & self.element_mask()));
		
		word_type mask(self.element_mask());
		auto const word_idx(idx / self.element_count_in_word());
		auto const el_idx(idx % self.element_count_in_word());
		auto const shift_amt(self.bits_before_element(el_idx));
		
		mask <<= shift_amt;
		val <<= shift_amt;
		mask = ~mask;
		val |= mask;
		
		word_type const retval(self.m_values[word_idx].fetch_and(val, order));
		return ((retval >> shift_amt) & self.element_mask());
	}
}}


namespace libbio {

	template <unsigned int t_bits, typename t_word, template <typename, unsigned int, typename> typename t_trait>
	std::ostream &operator<<(std::ostream &os, int_vector_tpl <t_bits, t_word, t_trait> const &vec)
	{
		ranges::copy(vec | ranges::view::transform([](auto val){ return +val; }), ranges::make_ostream_joiner(os, "\t"));
		return os;
	}
	
	
	// FIXME: allow comparing vectors with dynamic and static bit size, different word types.
	template <unsigned int t_bits, typename t_word, template <typename, unsigned int, typename> typename t_trait>
	bool int_vector_tpl <t_bits, t_word, t_trait>::operator==(int_vector_tpl const &other) const
	{
		// Different sizes.
		if (m_size != other.m_size)
			return false;
		
		// Empty vectors.
		if (0 == m_size)
			return true;
		
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

/*
 * Copyright (c) 2019 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_INT_VECTOR_WIDTH_HH
#define LIBBIO_INT_VECTOR_WIDTH_HH

#include <libbio/utility.hh>


namespace libbio { namespace detail {
	
	// Traits / mixins.
	
	template <unsigned int t_bits, typename t_word>
	struct int_vector_width_base
	{
		constexpr inline t_word extent_mask(std::size_t const extent_size);
	};
	
	
	template <unsigned int t_bits, typename t_word>
	class int_vector_width : public int_vector_width_base <t_bits, t_word>
	{
	public:
		typedef t_word		word_type;
		
		enum {
			WORD_BITS		= CHAR_BIT * sizeof(word_type),
			ELEMENT_BITS	= t_bits,
			ELEMENT_COUNT	= WORD_BITS / t_bits,
			ELEMENT_MASK	= (std::numeric_limits <word_type>::max() >> (WORD_BITS - t_bits))
		};
		
		int_vector_width() = default;
		
		int_vector_width(std::uint8_t /* ignored */):
			int_vector_width()
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
	class int_vector_width <0, t_word> : public int_vector_width_base <0, t_word>
	{
	public:
		typedef t_word		word_type;
		
		enum {
			WORD_BITS			= CHAR_BIT * sizeof(word_type),
			ELEMENT_BITS		= 0
		};
		
	protected:
		std::uint8_t	m_bits{};
		
	public:
		int_vector_width() = default;
		
		int_vector_width(std::uint8_t bits):
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
	
	
	template <unsigned int t_bits, typename t_word>
	constexpr auto int_vector_width_base <t_bits, t_word>::extent_mask(std::size_t const size) -> t_word
	{
		libbio_assert(size <= this->element_count_in_word());
		t_word const mask(~t_word(0));
		return mask >> ((this->element_count_in_word() - size) * this->element_bits());
	}
	
	
	template <unsigned int t_bits, typename t_word>
	constexpr auto int_vector_width <t_bits, t_word>::extent_mask(std::size_t const size, word_type const mask) -> word_type
	{
		libbio_assert(size <= this->element_count_in_word());
		return fill_bit_pattern <ELEMENT_BITS>(mask) >> ((this->element_count_in_word() - size) * this->element_bits());
	}
	
	
	template <typename t_word>
	auto int_vector_width <0, t_word>::extent_mask(std::size_t const size, word_type const mask) -> word_type
	{
		libbio_assert(size <= this->element_count_in_word());
		return fill_bit_pattern(mask, this->element_bits()) >> ((this->element_count_in_word() - size) * this->element_bits());
	}
}}

#endif

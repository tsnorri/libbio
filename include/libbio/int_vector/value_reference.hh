/*
 * Copyright (c) 2018–2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_INT_VECTOR_VALUE_REFERENCE_HH
#define LIBBIO_INT_VECTOR_VALUE_REFERENCE_HH

#include <cstddef>


namespace libbio { namespace detail {
	
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
		constexpr inline bool is_reference() const { return true; }
		
		word_type load() const { return m_vector->load(m_idx); }
		void assign_or(word_type val) { m_vector->assign_or(m_idx, val); }
		
		operator word_type() const { return load(); }
		int_vector_value_reference &operator|=(word_type val) { assign_or(val); return *this; }
	};
}}

#endif

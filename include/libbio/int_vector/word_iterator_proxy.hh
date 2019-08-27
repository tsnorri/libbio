/*
 * Copyright (c) 2018–2019 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_INT_VECTOR_WORD_ITERATOR_PROXY_HH
#define LIBBIO_INT_VECTOR_WORD_ITERATOR_PROXY_HH

#include <cstddef>
#include <libbio/int_vector/iterator.hh>


namespace libbio { namespace detail {
	
	template <typename t_vector>
	class int_vector_word_iterator_proxy_base
	{
	protected:
		t_vector	*m_vector{};
		
	public:
		int_vector_word_iterator_proxy_base() = default;
		int_vector_word_iterator_proxy_base(t_vector &vec):
			m_vector(&vec)
		{
		}
	};
	
	
	template <typename t_vector>
	class int_vector_word_iterator_proxy : public int_vector_word_iterator_proxy_base <t_vector>
	{
	public:
		typedef typename int_vector_iterator_traits <t_vector>::word_iterator			word_iterator;
		using int_vector_word_iterator_proxy_base <t_vector>::int_vector_word_iterator_proxy_base;
		word_iterator begin() const { return this->m_vector->word_begin(); }
		word_iterator end() const { return this->m_vector->word_end(); }
	};
	
	
	template <typename t_vector>
	class int_vector_reverse_word_iterator_proxy : public int_vector_word_iterator_proxy_base <t_vector>
	{
	public:
		typedef typename int_vector_iterator_traits <t_vector>::reverse_word_iterator	word_iterator;
		using int_vector_word_iterator_proxy_base <t_vector>::int_vector_word_iterator_proxy_base;
		word_iterator begin() const { return this->m_vector->word_rbegin(); }
		word_iterator end() const { return this->m_vector->word_rend(); }
	};
}}

#endif

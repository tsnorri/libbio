/*
 * Copyright (c) 2018–2019 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_INT_MATRIX_SLICE_HH
#define LIBBIO_INT_MATRIX_SLICE_HH

#include <libbio/matrix/slice.hh>
#include <libbio/int_vector/word_range.hh>


namespace libbio { namespace detail {
	
	template <typename t_matrix>
	class int_matrix_slice final : public matrix_slice <t_matrix>
	{
	public:
		typedef matrix_slice <t_matrix>							superclass;
		typedef typename superclass::matrix_type				matrix_type;
		typedef typename matrix_type::vector_type				vector_type;
		typedef typename matrix_type::word_type					word_type;
		typedef typename superclass::iterator					iterator;
		typedef typename superclass::iterator_range				iterator_range;
		typedef typename superclass::const_iterator				const_iterator;
		typedef typename superclass::const_iterator_range		const_iterator_range;

		typedef typename matrix_type::word_iterator				word_iterator;
		typedef typename matrix_type::const_word_iterator		const_word_iterator;
		typedef boost::iterator_range <word_iterator>			word_iterator_range;
		typedef boost::iterator_range <const_word_iterator>		const_word_iterator_range;
		
		typedef int_vector_word_range <vector_type>				word_range;
		typedef int_vector_word_range <vector_type const>		const_word_range;
		
	protected:
		template <typename t_word_range, typename t_caller>
		t_word_range to_word_range(t_caller &caller) const;

	public:
		using matrix_slice <t_matrix>::matrix_slice;
		
		// Check whether the starting position of the slice is word aligned.
		bool is_word_aligned() const { return 0 == this->m_slice.start() % this->matrix().element_count_in_word(); }
		
		word_range to_word_range() { return to_word_range <word_range>(*this); }
		const_word_range to_word_range() const { return to_const_word_range(); }
		const_word_range to_const_word_range() const { return to_word_range <const_word_range>(*this); }
		word_iterator word_begin() { return this->begin().to_vector_iterator().to_word_iterator(); }
	};
	
	
	template <typename t_matrix>
	template <typename t_word_range, typename t_caller>
	auto int_matrix_slice <t_matrix>::to_word_range(t_caller &caller) const -> t_word_range
	{
		libbio_assert(1 == caller.m_slice.stride());
		return t_word_range(caller.matrix().values(), caller.begin().to_vector_iterator(), caller.end().to_vector_iterator());
	}
}}

#endif

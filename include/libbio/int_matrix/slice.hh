/*
 * Copyright (c) 2018–2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_INT_MATRIX_SLICE_HH
#define LIBBIO_INT_MATRIX_SLICE_HH

#include <boost/range/iterator_range.hpp>
#include <libbio/assert.hh>
#include <libbio/int_vector/word_range.hh>
#include <libbio/matrix/slice.hh>
#include <span>


namespace libbio::detail {

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

		typedef std::span <word_type>							span;
		typedef std::span <word_type const>						const_span;

	protected:
		template <typename t_word_range, typename t_caller>
		t_word_range to_word_range(t_caller &caller) const;

		template <typename t_span, typename t_caller>
		t_span to_span(t_caller &caller) const;

		template <typename t_iterator>
		auto convert_to_word_iterator(t_iterator &&it) const { return it.to_vector_iterator().to_word_iterator(); }

	public:
		using matrix_slice <t_matrix>::matrix_slice;

		// Check whether the starting position of the slice is word aligned.
		bool is_word_aligned_at_start() const { return 0 == this->m_slice.start() % this->matrix().element_count_in_word() && 1 == this->m_slice.stride(); }

		bool is_word_aligned() const { return is_word_aligned_at_start() && 0 == (this->m_slice.start() + this->m_slice.size()) % this->matrix().element_count_in_word(); }

		word_range to_word_range() { return to_word_range <word_range>(*this); }
		const_word_range to_word_range() const { return to_const_word_range(); }
		const_word_range to_const_word_range() const { return to_word_range <const_word_range>(*this); }
		span to_span() { return to_span <span>(*this); }
		const_span to_span() const { return to_span <const_span>(*this); }
		const_span to_const_span() const { return to_span <const_span>(*this); }

		// These ultimately throw unless *this is word aligned. (See int_vector_iterator_base::to_word_iterator().)
		word_iterator word_begin() { return convert_to_word_iterator(this->begin()); }
		const_word_iterator word_begin() const { return convert_to_word_iterator(this->begin()); }
		const_word_iterator word_cbegin() const { return convert_to_word_iterator(this->begin()); }
		word_iterator word_end() { return convert_to_word_iterator(this->begin()); }
		const_word_iterator word_end() const { return convert_to_word_iterator(this->begin()); }
		const_word_iterator word_cend() const { return convert_to_word_iterator(this->begin()); }
	};


	template <typename t_matrix>
	template <typename t_word_range, typename t_caller>
	auto int_matrix_slice <t_matrix>::to_word_range(t_caller &caller) const -> t_word_range
	{
		libbio_assert(1 == caller.m_slice.stride());
		return t_word_range(caller.matrix().values(), caller.begin().to_vector_iterator(), caller.end().to_vector_iterator());
	}


	template <typename t_matrix>
	template <typename t_span, typename t_caller>
	auto int_matrix_slice <t_matrix>::to_span(t_caller &caller) const -> t_span
	{
		libbio_assert(caller.is_word_aligned()); // Also require that the last element is word aligned.
		libbio_assert(1 == caller.m_slice.stride());
		return t_span(convert_to_word_iterator(caller.begin()), convert_to_word_iterator(caller.end()));
	}
}

#endif

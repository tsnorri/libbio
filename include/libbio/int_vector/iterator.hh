/*
 * Copyright (c) 2018–2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_INT_VECTOR_ITERATOR_HH
#define LIBBIO_INT_VECTOR_ITERATOR_HH

#include <boost/iterator/iterator_facade.hpp>
#include <cstddef>
#include <libbio/assert.hh>
#include <limits>
#include <stdexcept>
#include <type_traits>


namespace libbio::detail {

	template <typename t_vector, bool t_is_const = std::is_const_v <t_vector>>
	struct int_vector_iterator_traits
	{
		typedef typename t_vector::reference				reference;
		typedef typename t_vector::iterator					iterator;
		typedef typename t_vector::word_iterator			word_iterator;
		typedef typename t_vector::reverse_word_iterator	reverse_word_iterator;
	};

	template <typename t_vector>
	struct int_vector_iterator_traits <t_vector, true>
	{
		typedef typename t_vector::const_reference				reference;
		typedef typename t_vector::const_iterator				iterator;
		typedef typename t_vector::const_word_iterator			word_iterator;
		typedef typename t_vector::const_reverse_word_iterator	reverse_word_iterator;
	};


	template <typename t_vector>
	class int_vector_iterator_base
	{
	public:
		typedef int_vector_iterator_traits <t_vector>	traits;
		typedef typename traits::word_iterator			word_iterator;
		typedef typename t_vector::reference_proxy		reference_proxy;

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
		std::size_t word_index() const { return m_idx / m_vector->element_count_in_word(); }
		std::size_t word_offset() const { return m_idx % m_vector->element_count_in_word(); }

		virtual void advance(std::ptrdiff_t) = 0;
		bool equal(int_vector_iterator_base const &other) const { return m_vector == other.m_vector && m_idx == other.m_idx; }

		void increment() { advance(1); }
		void decrement() { advance(-1); }
		typename traits::reference dereference() const { return m_vector->operator()(m_idx); }

		reference_proxy to_reference_proxy() const { return reference_proxy(*m_vector, m_idx); }
		word_iterator to_containing_word_iterator() const { return m_vector->word_begin() + m_idx / m_vector->element_count_in_word(); }
		word_iterator to_word_iterator() const
		{
			if (0 != word_offset())
				throw std::runtime_error("Unable to convert to word_iterator");
			return m_vector->word_begin() + m_idx / m_vector->element_count_in_word();
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
}

#endif

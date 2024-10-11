/*
 * Copyright (c) 2018-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_MATRIX_ITERATOR_HH
#define LIBBIO_MATRIX_ITERATOR_HH

#include <boost/iterator/iterator_facade.hpp>
#include <iterator>
#include <ostream>
#include <libbio/assert.hh>


namespace libbio::detail {
	
	template <typename t_iterator>
	class matrix_iterator : public boost::iterator_facade <
		matrix_iterator <t_iterator>,
		std::remove_reference_t <typename std::iterator_traits <t_iterator>::reference>,
		boost::random_access_traversal_tag
	>
	{
		friend class boost::iterator_core_access;
		
	private:
		typedef typename std::iterator_traits <t_iterator>::reference		iterator_reference_type;
		
	public:
		typedef typename std::remove_reference_t <iterator_reference_type>	value_type;
		
	private:
		t_iterator			m_it{};
		std::ptrdiff_t		m_stride{1}; // Need signed type for division in distance_to().

	public:
		matrix_iterator() = default;
		
		template <typename t_matrix>
		matrix_iterator(t_matrix &matrix, std::size_t start, std::size_t stride):
			m_it(matrix.begin() + start),
			m_stride(stride)
		{
		}
		
		template <
			typename t_other_iterator,
			typename = std::enable_if_t <std::is_convertible_v <t_other_iterator *, t_iterator *>>
		> matrix_iterator(matrix_iterator <t_other_iterator> const &other):
			m_it(other.m_it),
			m_stride(other.m_stride)
		{
		}
		
	private:
		value_type &dereference() const { return *m_it; }
		bool equal(matrix_iterator const &other) const { return m_it == other.m_it && m_stride == other.m_stride; }
		void increment() { advance(1); }
		void decrement() { advance(-1); }
		void advance(std::ptrdiff_t const diff) { std::advance(m_it, m_stride * diff); }
		
		std::ptrdiff_t distance_to(matrix_iterator const &other) const
		{
			auto const dist(std::distance(m_it, other.m_it));
			libbio_assert(0 == dist % m_stride);
			auto const retval(dist / m_stride);
			return retval;
		}
	};
	
	
	template <typename t_iterator>
	std::ostream &operator<<(std::ostream &stream, matrix_iterator <t_iterator> const &it) { stream << "(matrix iterator)"; return stream; }
}

#endif

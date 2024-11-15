/*
 * Copyright (c) 2018-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_MATRIX_SLICE_HH
#define LIBBIO_MATRIX_SLICE_HH

#include <boost/range.hpp>
#include <cstddef>
#include <iostream>
#include <range/v3/algorithm/copy.hpp>
#include <range/v3/iterator/stream_iterators.hpp>
#include <range/v3/view/transform.hpp>
#include <valarray>


namespace libbio::detail {

	template <typename t_matrix>
	class matrix_slice
	{
	public:
		typedef t_matrix										matrix_type;
		typedef typename matrix_type::reference					reference;
		typedef typename matrix_type::const_reference			const_reference;
		typedef typename matrix_type::iterator					iterator;
		typedef typename matrix_type::const_iterator			const_iterator;
		typedef typename matrix_type::matrix_iterator			matrix_iterator;
		typedef typename matrix_type::const_matrix_iterator		const_matrix_iterator;
		typedef boost::iterator_range <iterator>				iterator_range;
		typedef boost::iterator_range <const_iterator>			const_iterator_range;
		typedef boost::iterator_range <matrix_iterator>			matrix_iterator_range;
		typedef boost::iterator_range <const_matrix_iterator>	const_matrix_iterator_range;

	protected:
		t_matrix	*m_matrix{nullptr};
		std::slice	m_slice{};

	public:
		matrix_slice() = default;
		matrix_slice(t_matrix &matrix, std::slice const &slice):
			m_matrix(&matrix),
			m_slice(slice)
		{
		}

		matrix_type &matrix() { return *m_matrix; }
		matrix_type const &matrix() const { return *m_matrix; }

		std::size_t size() const { return m_slice.size(); }

		reference operator[](std::size_t const idx) { libbio_assert_lt(idx, size()); return *(begin() + idx); }
		const_reference operator[](std::size_t const idx) const { libbio_assert_lt(idx, size()); return *(begin() + idx); }

		matrix_iterator begin() { return matrix_iterator(*m_matrix, m_slice.start(), m_slice.stride()); }
		matrix_iterator end() { return matrix_iterator(*m_matrix, m_slice.start() + m_slice.size() * m_slice.stride(), m_slice.stride()); }
		matrix_iterator_range range() { return boost::make_iterator_range(begin(), end()); }
		const_matrix_iterator begin() const { return cbegin(); }
		const_matrix_iterator end() const { return cend(); }
		const_matrix_iterator_range range() const { return const_range(); }
		const_matrix_iterator cbegin() const { return const_matrix_iterator(*m_matrix, m_slice.start(), m_slice.stride()); }
		const_matrix_iterator cend() const { return const_matrix_iterator(*m_matrix, m_slice.start() + m_slice.size() * m_slice.stride(), m_slice.stride()); }
		const_matrix_iterator_range const_range() const { return boost::make_iterator_range(cbegin(), cend()); }

		void output_to_stderr() { std::cerr << *this << std::endl; } // For debugging.
	};


	template <typename t_matrix>
	std::ostream &operator<<(std::ostream &os, matrix_slice <t_matrix> const &slice)
	{
		ranges::copy(slice | ranges::views::transform([](auto val){ return +val; }), ranges::make_ostream_joiner(os, "\t"));
		return os;
	}
}

#endif

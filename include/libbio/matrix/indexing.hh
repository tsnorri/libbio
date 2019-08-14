/*
 * Copyright (c) 2018–2019 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_MATRIX_INDEXING_HH
#define LIBBIO_MATRIX_INDEXING_HH

#include <libbio/assert.hh>


namespace libbio { namespace detail {
	
	template <typename t_matrix>
	inline std::size_t matrix_index(t_matrix const &matrix, std::size_t const y, std::size_t const x)
	{
		/* Column major order. */
		libbio_assert(y < matrix.m_stride);
		libbio_assert(x < matrix.m_columns);
		libbio_assert(x < matrix.m_data.size() / matrix.m_stride);
		std::size_t const retval(x * matrix.m_stride + y);
		libbio_assert(retval < matrix.m_data.size());
		return retval;
	}
}}

#endif

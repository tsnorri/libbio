/*
 * Copyright (c) 2018–2023 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_MATRIX_INDEXING_HH
#define LIBBIO_MATRIX_INDEXING_HH

#include <libbio/assert.hh>


namespace libbio { namespace detail {
	
	inline std::size_t matrix_index(std::size_t const y, std::size_t const x, std::size_t const stride)
	{
		return x * stride + y;
	}
	
	
	template <typename t_matrix>
	inline std::size_t matrix_index(t_matrix const &matrix, std::size_t const y, std::size_t const x)
	{
		/* Column major order. */
		libbio_assert_lt(y, matrix.m_stride);
		libbio_assert_lt(x, matrix.number_of_columns());
		libbio_assert_lt_msg(x, matrix.m_data.size() / matrix.m_stride, "x: ", x, " matrix.m_data.size(): ", matrix.m_data.size(), " matrix.m_stride: ", matrix.m_stride);
		auto const retval(matrix_index(y, x, matrix.m_stride));
		libbio_assert_lt(retval, matrix.m_data.size());
		return retval;
	}
}}

#endif

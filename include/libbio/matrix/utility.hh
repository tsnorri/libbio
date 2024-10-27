/*
 * Copyright (c) 2018-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_MATRIX_UTILITY_HH
#define LIBBIO_MATRIX_UTILITY_HH

#include <cstddef>
#include <libbio/assert.hh>
#include <valarray>


namespace libbio::matrices {

#if 0
	template <typename t_matrix> typename t_matrix::slice_type row(t_matrix &matrix, std::size_t const row, std::size_t const first, std::size_t const limit);
	template <typename t_matrix> typename t_matrix::slice_type column(t_matrix &matrix, std::size_t const column, std::size_t const first, std::size_t const limit);
	template <typename t_matrix> typename t_matrix::const_slice_type const_row(t_matrix const &matrix, std::size_t const row, std::size_t const first, std::size_t const limit);
	template <typename t_matrix> typename t_matrix::const_slice_type const_column(t_matrix const &matrix, std::size_t const column, std::size_t const first, std::size_t const limit);
#endif


	template <typename t_matrix>
	typename t_matrix::slice_type row(t_matrix &matrix, std::size_t const row, std::size_t const first, std::size_t const limit)
	{
		libbio_assert(limit <= matrix.number_of_columns());
		std::slice const slice(matrix.idx(row, first), limit - first, matrix.stride());
		return typename t_matrix::slice_type(matrix, slice);
	}


	template <typename t_matrix>
	typename t_matrix::slice_type column(t_matrix &matrix, std::size_t const column, std::size_t const first, std::size_t const limit)
	{
		libbio_assert(limit <= matrix.number_of_rows());
		std::slice const slice(matrix.idx(first, column), limit - first, 1);
		return typename t_matrix::slice_type(matrix, slice);
	}


	template <typename t_matrix>
	typename t_matrix::const_slice_type const_row(t_matrix const &matrix, std::size_t const row, std::size_t const first, std::size_t const limit)
	{
		libbio_assert(limit <= matrix.number_of_columns());
		std::slice const slice(matrix.idx(row, first), limit - first, matrix.stride());
		return typename t_matrix::const_slice_type(matrix, slice);
	}


	template <typename t_matrix>
	typename t_matrix::const_slice_type const_column(t_matrix const &matrix, std::size_t const column, std::size_t const first, std::size_t const limit)
	{
		libbio_assert(limit <= matrix.number_of_rows());
		std::slice const slice(matrix.idx(first, column), limit - first, 1);
		return typename t_matrix::const_slice_type(matrix, slice);
	}
}

#endif

/*
 * Copyright (c) 2019 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_INT_MATRIX_CEREAL_SERIALIZATION_HH
#define LIBBIO_INT_MATRIX_CEREAL_SERIALIZATION_HH

#include <cereal/cereal.hpp>
#include <libbio/int_matrix/int_matrix.hh>


namespace libbio {
	
	template <typename t_archive, unsigned int t_bits, typename t_word, template <typename, unsigned int, typename> typename t_trait>
	inline void save(t_archive &archive, int_matrix_tpl <t_bits, t_word, t_trait> const &mat)
	{
		std::size_t const columns(mat.number_of_columns());
		archive(mat.m_data, columns, mat.m_stride);
	}
	
	
	template <typename t_archive, unsigned int t_bits, typename t_word, template <typename, unsigned int, typename> typename t_trait>
	inline void load(t_archive &archive, int_matrix_tpl <t_bits, t_word, t_trait> &mat)
	{
#ifndef LIBBIO_NDEBUG
		archive(mat.m_data, mat.m_columns, mat.m_stride);
#else
		std::size_t columns(0);
		archive(mat.m_data, columns, mat.m_stride);
#endif
	}
}

#endif

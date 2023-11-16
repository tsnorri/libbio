/*
 * Copyright (c) 2019-2023 Tuukka Norri
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
		archive(mat.m_data, mat.m_stride);
	}
	
	
	template <typename t_archive, unsigned int t_bits, typename t_word, template <typename, unsigned int, typename> typename t_trait>
	inline void load(t_archive &archive, int_matrix_tpl <t_bits, t_word, t_trait> &mat)
	{
		archive(mat.m_data, mat.m_stride);
	}
}

#endif

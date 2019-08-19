/*
 * Copyright (c) 2019 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_INT_VECTOR_CEREAL_SERIALIZATION_HH
#define LIBBIO_INT_VECTOR_CEREAL_SERIALIZATION_HH

#include <cereal/cereal.hpp>
#include <cereal/types/vector.hpp>
#include <libbio/int_vector/int_vector.hh>
#include <type_traits>


namespace libbio {
	
	template <typename t_archive, unsigned int t_bits, typename t_word, template <typename, unsigned int, typename> typename t_trait>
	inline void serialize(t_archive &archive, int_vector_tpl <t_bits, t_word, t_trait> &vec)
	{
		archive(vec.m_values, vec.m_size);
		if constexpr (std::remove_reference_t <decltype(vec)>::ELEMENT_BITS == 0)
			archive(vec.m_bits);
	}
}

#endif

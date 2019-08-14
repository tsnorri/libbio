/*
 * Copyright (c) 2019 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_INT_VECTOR_CEREAL_SERIALIZATION_HH
#define LIBBIO_INT_VECTOR_CEREAL_SERIALIZATION_HH

#include <libbio/int_vector/int_vector.hh>


namespace libbio {
	
	template <typename t_archive, unsigned int t_bits, typename t_word, template <typename, unsigned int, typename> typename t_trait>
	void serialize(t_archive &archive, int_vector_tpl <t_bits, t_word, t_trait> &vec)
	{
		archive(vec.m_data, vec.m_size);
		if constexpr (decltype(vec)::ELEMENT_BITS == 0)
			archive(vec.m_bits);
	}
	
	template <typename t_archive, unsigned int t_bits, typename t_word, template <typename, unsigned int, typename> typename t_trait>
	void serialize(t_archive &archive, int_vector_tpl <t_bits, t_word, t_trait> &vec, std::uint32_t const version)
	{
		serialize(archive, vec);
	}
}

#endif

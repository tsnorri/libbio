/*
 * Copyright (c) 2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_INT_VECTOR_SIZE_CALCULATION_HH
#define LIBBIO_INT_VECTOR_SIZE_CALCULATION_HH

#include <libbio/int_vector/int_vector.hh>
#include <libbio/size_calculator.hh>


namespace libbio::size_calculation {

	template <unsigned int t_bits, typename t_word, template <typename, unsigned int, typename> typename t_trait>
	struct value_size_calculator <int_vector_tpl <t_bits, t_word, t_trait>>
	{
		typedef int_vector_tpl <t_bits, t_word, t_trait> value_type;

		void operator()(size_calculator &sc, size_calculator_entry &entry, value_type const &val) const
		{
			calculate_value_size(sc, entry, val.m_values);
		}
	};
}

#endif

/*
 * Copyright (c) 2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_INT_MATRIX_SIZE_CALCULATION_HH
#define LIBBIO_INT_MATRIX_SIZE_CALCULATION_HH

#include <libbio/int_matrix/int_matrix.hh>
#include <libbio/int_vector/size_calculation.hh>
#include <libbio/size_calculator.hh>


namespace libbio::size_calculation {

	template <unsigned int t_bits, typename t_word, template <typename, unsigned int, typename> typename t_trait>
	struct value_size_calculator <int_matrix_tpl <t_bits, t_word, t_trait>>
	{
		typedef int_matrix_tpl <t_bits, t_word, t_trait> value_type;

		void operator()(size_calculator &sc, size_calculator_entry &entry, value_type const &val) const
		{
			calculate_value_size(sc, entry, val.values());
			entry.size += sizeof(std::size_t);
		}
	};
}

#endif

/*
 * Copyright (c) 2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_INT_MATRIX_SIZE_CALCULATION_HH
#define LIBBIO_INT_MATRIX_SIZE_CALCULATION_HH

#include <cstddef>
#include <libbio/int_matrix/int_matrix.hh>
#include <libbio/int_vector/size_calculation.hh>
#include <libbio/size_calculator.hh>


namespace libbio::size_calculation {

	template <unsigned int t_bits, typename t_word, template <typename, unsigned int, typename> typename t_trait>
	struct value_size_calculator <int_matrix_tpl <t_bits, t_word, t_trait>>
	{
		typedef int_matrix_tpl <t_bits, t_word, t_trait> value_type;

		void operator()(size_calculator &sc, entry_index_type const entry_idx, value_type const &val) const
		{
			calculate_value_size(sc, entry_idx, val.values());
			sc.entries[entry_idx].size += sizeof(std::size_t);
		}
	};
}

#endif

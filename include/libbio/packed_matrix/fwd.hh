/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_PACKED_MATRIX_FWD_HH
#define LIBBIO_PACKED_MATRIX_FWD_HH

#include <cstdint>


namespace libbio {
	
	template <unsigned int t_bits, typename t_word = std::uint64_t>
	class packed_matrix;
}

#endif

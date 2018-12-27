/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_TYPE_TRAITS_HH
#define LIBBIO_TYPE_TRAITS_HH

#include <type_traits>


namespace libbio {

	template <typename t_type>
	inline constexpr bool is_integral_ref_v = std::is_integral_v <std::remove_reference_t <t_type>>;
}

#endif

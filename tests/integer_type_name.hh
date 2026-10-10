/*
 * Copyright (c) 2026 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_TESTS_INTEGER_TYPE_NAME_HH
#define LIBBIO_TESTS_INTEGER_TYPE_NAME_HH

#include <cstdint>


namespace libbio::tests {

	template <typename t_type> struct integer_type_name {};

#define LIBBIO_INTEGER_TYPE_NAME_SPECIALISATION(TYPE) template <> struct integer_type_name <TYPE> { constexpr static char const value[]{#TYPE}; };

	LIBBIO_INTEGER_TYPE_NAME_SPECIALISATION(std::int8_t);
	LIBBIO_INTEGER_TYPE_NAME_SPECIALISATION(std::int16_t);
	LIBBIO_INTEGER_TYPE_NAME_SPECIALISATION(std::int32_t);
	LIBBIO_INTEGER_TYPE_NAME_SPECIALISATION(std::int64_t);

	LIBBIO_INTEGER_TYPE_NAME_SPECIALISATION(std::uint8_t);
	LIBBIO_INTEGER_TYPE_NAME_SPECIALISATION(std::uint16_t);
	LIBBIO_INTEGER_TYPE_NAME_SPECIALISATION(std::uint32_t);
	LIBBIO_INTEGER_TYPE_NAME_SPECIALISATION(std::uint64_t);

#undef LIBBIO_INTEGER_TYPE_NAME_SPECIALISATION


}

#endif

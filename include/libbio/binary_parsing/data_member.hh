/*
 * Copyright (c) 2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_BINARY_PARSING_DATA_MEMBER_HH
#define LIBBIO_BINARY_PARSING_DATA_MEMBER_HH

#include <cstddef>	// std::size_t


namespace libbio::binary_parsing {
	
	template <typename t_type, typename t_class>
	struct data_member
	{
		typedef t_class	class_type;
		typedef t_type	type;
		
		type class_type::*value{};
		
		constexpr data_member(type (class_type::*value_)):
			value(value_)
		{
		}
		
		constexpr std::size_t size() const { return sizeof(type); }
	};
}

#endif

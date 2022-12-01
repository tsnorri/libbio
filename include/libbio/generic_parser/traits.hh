/*
 * Copyright (c) 2022 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_GENERIC_PARSER_TRAITS_HH
#define LIBBIO_GENERIC_PARSER_TRAITS_HH

#include <libbio/generic_parser/delimiter.hh>
#include <type_traits>							// std::conditional_t


namespace libbio::parsing::traits {
	
	// Not sure if there are any other kinds of input than delimited that we can parse but we’ll see.
	template <typename t_field_sep, typename t_line_sep = t_field_sep>
	struct delimited
	{
		template <std::size_t t_field_count>
		struct trait
		{
			template <std::size_t t_i>
			constexpr static inline enum field_position const field_position_()
			{
				if constexpr (1 == t_field_count)
				{
					return field_position::initial_ | field_position::final_;
				}
				else
				{
					switch (t_i)
					{
						case 0:
							return field_position::initial_;
						case (t_field_count - 1):
							return field_position::final_;
						default:
							return field_position::middle_;
					}
				}
			}
			
			template <std::size_t t_i>
			constexpr static inline enum field_position const field_position{field_position_ <t_i>()};
			
			template <std::size_t t_i>
			using delimiter = std::conditional_t <t_i == t_field_count - 1, t_line_sep, t_field_sep>;
		};
	};
}

#endif

/*
 * Copyright (c) 2022-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_GENERIC_PARSER_TRAITS_HH
#define LIBBIO_GENERIC_PARSER_TRAITS_HH

#include <libbio/generic_parser/delimiter.hh>
#include <libbio/generic_parser/field_position.hh>
#include <libbio/generic_parser/fields.hh>
#include <type_traits>								// std::conditional_t


namespace libbio::parsing::traits {
	
	// Not sure if there are any other kinds of input than delimited that we can parse but we’ll see.
	template <typename t_field_sep, typename t_line_sep = t_field_sep>
	struct delimited
	{
		template <std::size_t t_field_count>
		struct trait
		{
			typedef t_field_sep	field_separator_type;
			typedef t_line_sep	line_separator_type;
			
			template <std::size_t t_i, typename t_next_field>
			constexpr static inline enum field_position const field_position_()
			{
				if constexpr (1 == t_field_count)
				{
					return field_position::initial_ | field_position::final_;
				}
				else
				{
					enum field_position retval{field_position::none_};
					switch (t_i)
					{
						case 0:
							retval = field_position::initial_;
							break;
						case (t_field_count - 1):
							retval = field_position::final_;
							break;
						default:
							retval = field_position::middle_;
							break;
					}
					
					if constexpr (is_optional_v <t_next_field>)
						retval |= field_position::final_;
					
					return retval;
				}
			}
			
			template <std::size_t t_i, typename t_next_field>
			constexpr static inline enum field_position const field_position{field_position_ <t_i, t_next_field>()};
			
			
			template <typename t_field, typename t_next_field, std::size_t t_i>
			struct delimiter_
			{
				static_assert(!is_optional_v <t_field> || std::is_void_v <t_next_field>, "Optional field can only appear as the final one");
				typedef std::conditional_t <
					is_optional_v <t_next_field>,
					join_delimiters_t <field_separator_type, line_separator_type>,
					std::conditional_t <t_i == t_field_count - 1, line_separator_type, field_separator_type>
				> type;
			};
			
			template <typename t_field, typename t_next_field, std::size_t t_i>
			using delimiter = delimiter_ <t_field, t_next_field, t_i>::type;
		};
	};
}

#endif

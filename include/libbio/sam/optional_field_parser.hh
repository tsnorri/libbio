/*
 * Copyright (c) 2023 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_SAM_OPTIONAL_FIELD_PARSER_HH
#define LIBBIO_SAM_OPTIONAL_FIELD_PARSER_HH

#include <libbio/sam/input_range.hh>
#include <libbio/sam/optional_field.hh>


namespace libbio::sam {
	void read_optional_fields(input_range_base &range, optional_field &dst);
}


namespace libbio::sam::fields {
	
	struct optional_field
	{
		constexpr static bool const is_optional{true};
		
		template <bool t_should_copy>
		using value_type = sam::optional_field;
		
		constexpr void clear_value(sam::optional_field &dst) const { dst.clear(); }
		
		template <typename t_delimiter, parsing::field_position t_field_position, typename t_range>
		constexpr parsing::parsing_result parse(t_range &range, sam::optional_field &dst) const
		{
			dst.clear(); // FIXME: Unneeded here?
			
			if constexpr (any(parsing::field_position::initial_ & t_field_position))
			{
				if (range.is_at_end())
					return {};
			}
			
			read_optional_fields(range, dst);
			
			// If read_optional_fields_() does not throw, the final newline has been matched.
			constexpr auto const retval{t_delimiter::template index_of_v <parsing::delimiter_type('\n')>};
			static_assert(parsing::INVALID_DELIMITER_INDEX != retval);
			return retval;
		}
	};

	static_assert(parsing::is_optional_v <optional_field>);
}

#endif

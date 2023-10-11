/*
 * Copyright (c) 2023 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_SAM_CIGAR_FIELD_PARSER_HH
#define LIBBIO_SAM_CIGAR_FIELD_PARSER_HH

#include <libbio/assert.hh>
#include <libbio/sam/cigar.hh>
#include <libbio/generic_parser/errors.hh>	// parsing::errors::unexpected_character
#include <libbio/generic_parser/fields.hh>
#include <vector>


namespace libbio::sam::fields {
	
	struct cigar_field
	{
		typedef std::vector <cigar_run>	cigar_vector;
		typedef cigar_run::count_type	cigar_count_type;
		
		
		template <bool t_should_copy>
		using value_type = cigar_vector;
		
		
		template <typename t_range>
		constexpr inline cigar_run parse_one(t_range &range) const
		{
			cigar_count_type count{};
			parsing::fields::integer <cigar_count_type> integer_field;
			integer_field.parse_value <parsing::field_position::middle_>(range, count);
			
			char cigar_op{};
			parsing::fields::character character_field;
			character_field.parse_value <parsing::field_position::middle_>(range, cigar_op);
			auto const cigar_op_(make_cigar_operation(cigar_op, [](auto const op){
				throw parsing::parse_error_tpl(parsing::errors::unexpected_character(op));
			}));
			
			return cigar_run{cigar_op_, count};
		}
		
		
		// FIXME: Combine with the CIGAR parser for SeqAn 3; the following function is (almost?) identical with the one written earlier.
		template <typename t_delimiter, parsing::field_position t_field_position = parsing::field_position::middle_, typename t_range>
		constexpr inline parsing::parsing_result parse(t_range &range, cigar_vector &dst) const
		{
			dst.clear();
			
			if constexpr (any(parsing::field_position::initial_ & t_field_position))
			{
				if (range.is_at_end())
					return {};
				goto continue_parsing;
			}
			
			while (!range.is_at_end())
			{
			continue_parsing:
				dst.emplace_back(parse_one(range));
				
				auto const cc(*range.it);
				if (auto const idx{t_delimiter::matching_index(cc)}; t_delimiter::size() != idx)
				{
					++range.it;
					return {idx};
				}
			}
			
			if constexpr (t_field_position == parsing::field_position::final_)
			{
				if (range.is_at_end())
					return {parsing::INVALID_DELIMITER_INDEX};
			}
			else
			{
				if (range.is_at_end())
					throw parsing::parse_error_tpl(parsing::errors::unexpected_eof());
			}
			
			libbio_fail("Should not be reached");
			return {};
		}
	};
}

#endif

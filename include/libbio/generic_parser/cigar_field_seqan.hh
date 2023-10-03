/*
 * Copyright (c) 2022-2023 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_GENERIC_PARSER_CIGAR_FIELD_SEQAN_HH
#define LIBBIO_GENERIC_PARSER_CIGAR_FIELD_SEQAN_HH

#include <libbio/assert.hh>
#include <libbio/generic_parser/errors.hh>
#include <libbio/generic_parser/fields.hh>
#include <seqan3/alphabet/cigar/cigar.hpp>
#include <tuple>
#include <vector>


namespace libbio::parsing::fields::detail {
	
	// I’m not sure how to get the types from seqan3::type_list, so here is a helper for that.
	// (Also I don’t know if there is syntax for generalising this for any template that takes a parameter pack.)
	
	template <typename t_type>
	struct type_list_to_tuple {};
	
	template <typename... t_types>
	struct type_list_to_tuple <seqan3::type_list <t_types...>>
	{
		typedef std::tuple <t_types...> type;
	};
	
	template <typename t_type>
	using type_list_to_tuple_t = type_list_to_tuple <t_type>::type;
}


namespace libbio::parsing::fields {
	
	struct cigar
	{
		typedef std::vector <seqan3::cigar>	cigar_vector;
	
		typedef detail::type_list_to_tuple_t <seqan3::cigar::seqan3_required_types>	cigar_component_types;
		typedef std::tuple_element_t <0, cigar_component_types>						cigar_count_type;
		
		
		template <bool t_should_copy>
		using value_type = cigar_vector;
		
		
		template <typename t_range>
		constexpr inline seqan3::cigar parse_one(t_range &range) const
		{
			using seqan3::operator""_cigar_operation;
			
			seqan3::cigar retval{};
			cigar_count_type count{};
			char cigar_op{};
			integer <cigar_count_type> integer_field;
			character character_field;
			
			integer_field.parse_value <field_position::middle_>(range, count);
			retval = count;
			
			character_field.parse_value <field_position::middle_>(range, cigar_op);
			switch (cigar_op)
			{
				case 'M':
					retval = 'M'_cigar_operation;
					break;
				case 'I':
					retval = 'I'_cigar_operation;
					break;
				case 'D':
					retval = 'D'_cigar_operation;
					break;
				case 'N':
					retval = 'N'_cigar_operation;
					break;
				case 'S':
					retval = 'S'_cigar_operation;
					break;
				case 'H':
					retval = 'H'_cigar_operation;
					break;
				case 'P':
					retval = 'P'_cigar_operation;
					break;
				case '=':
					retval = '='_cigar_operation;
					break;
				case 'X':
					retval = 'X'_cigar_operation;
					break;
				default:
					throw parse_error_tpl(errors::unexpected_character(cigar_op));
			}
			
			return retval;
		}
	
	
		template <typename t_delimiter, field_position t_field_position = field_position::middle_, typename t_range>
		constexpr inline parsing_result parse(t_range &range, cigar_vector &dst) const
		{
			dst.clear();
			
			if constexpr (any(field_position::initial_ & t_field_position))
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
			
			if constexpr (t_field_position == field_position::final_)
			{
				if (range.is_at_end())
					return {idx};
			}
			else
			{
				if (range.is_at_end())
					throw parse_error_tpl(errors::unexpected_eof());
			}
			
			libbio_fail("Should not be reached");
			return {};
		}
	};
}

#endif

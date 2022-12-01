/*
 * Copyright (c) 2022 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_GENERIC_PARSER_FIELDS_HH
#define LIBBIO_GENERIC_PARSER_FIELDS_HH

#include <boost/spirit/home/x3.hpp>
#include <libbio/generic_parser/delimiter.hh>
#include <libbio/generic_parser/errors.hh>
#include <libbio/generic_parser/filters.hh>
#include <libbio/generic_parser/iterators.hh>
#include <string>
#include <string_view>
#include <type_traits>							// std::is_signed_v


// Clang 14 follows the standard in the sense that goto is not allowed in constexpr functions
// (even in if !consteval block).
#if defined(__GNUG__) && !defined(__clang__)
#	define LIBBIO_CONSTEXPR_WITH_GOTO	constexpr
#else
#	define LIBBIO_CONSTEXPR_WITH_GOTO
#endif


namespace libbio::parsing {
	
	enum class field_position : std::uint8_t
	{
		initial_ = 0x1,
		middle_ = 0x2,
		final_ = 0x4
	};
	
	constexpr inline field_position operator|(field_position const lhs, field_position const rhs)
	{
		return static_cast <field_position>(std::to_underlying(lhs) | std::to_underlying(rhs));
	}
	
	constexpr inline field_position operator&(field_position const lhs, field_position const rhs)
	{
		return static_cast <field_position>(std::to_underlying(lhs) | std::to_underlying(rhs));
	}
	
	constexpr inline bool any(field_position const fp)
	{
		return std::to_underlying(fp) ? true : false;
	}
}


namespace libbio::parsing::fields::detail {
	
	template <bool t_should_copy>
	struct text_value_type_tpl
	{
		typedef std::string_view	type;
	};

	template <>
	struct text_value_type_tpl <true>
	{
		typedef std::string			type;
	};
}


namespace libbio::parsing::fields {

	// Context-free grammars are not yet handled.

	struct skip
	{
		template <bool t_should_copy>
		using value_type = void;


		template <typename t_delimiter, field_position t_field_position = field_position::middle_, typename t_range>
		LIBBIO_CONSTEXPR_WITH_GOTO inline bool parse(t_range &range) const
		{
			if constexpr (any(field_position::initial_ & t_field_position))
			{
				if (range.is_at_end())
					return false;
				
				goto continue_parsing;
			}
	
			while (!range.is_at_end())
			{
			continue_parsing:
				if (t_delimiter::matches(*range.it))
				{
					++range.it;
					return true;
				}
				++range.it;
			}
		
			if constexpr (any(field_position::final_ & t_field_position))
			{
				// t_delimiter not matched.
				throw parse_error_tpl(errors::unexpected_eof());
			}
		
			return false;
		}
	};


	template <typename t_character_filter = filters::no_op>
	struct text
	{
		template <bool t_should_copy>
		using value_type = typename detail::text_value_type_tpl <t_should_copy>::type;


		template <typename t_delimiter, field_position t_field_position = field_position::middle_, typename t_range>
		requires (!t_range::is_contiguous)
		LIBBIO_CONSTEXPR_WITH_GOTO inline bool parse(t_range &range, std::string &dst) const
		{
			dst.clear();
		
			if constexpr (any(field_position::initial_ & t_field_position))
			{
				if (range.is_at_end())
					return false;
				goto continue_parsing;
			}
	
			while (!range.is_at_end())
			{
			continue_parsing:
				auto const cc(*range.it);
				if (t_delimiter::matches(cc))
				{
					++range.it;
					return true;
				}
				else if (!t_character_filter::check(cc))
				{
					throw parse_error_tpl(errors::unexpected_character(cc));
				}
		
				dst.push_back(cc);
				++range.it;
			}
			
			// t_delimiter not matched.
			if constexpr (any(field_position::final_ & t_field_position))
				return true;
			else
				throw parse_error_tpl(errors::unexpected_eof());
		}


		template <typename t_delimiter, field_position t_field_position = field_position::middle_, typename t_range, typename t_dst>
		requires t_range::is_contiguous
		LIBBIO_CONSTEXPR_WITH_GOTO inline bool parse_(t_range &range, t_dst &dst) const
		{
			auto begin(range.it); // Copy.
	
			if constexpr (any(field_position::initial_ & t_field_position))
			{
				if (range.is_at_end())
					return false;
				goto continue_parsing;
			}
	
			while (!range.is_at_end())
			{
			continue_parsing:
				auto const cc(*range.it);
				if (t_delimiter::matches(cc))
				{
					dst = std::string_view(begin, range.it);
					++range.it;
					return true;
				}
				else if (!t_character_filter::check(cc))
				{
					throw parse_error_tpl(errors::unexpected_character(cc));
				}
			
				++range.it;
			}
			
			// t_delimiter not matched.
			if constexpr (any(field_position::final_ & t_field_position))
			{
				dst = std::string_view(begin, range.it);
				return true;
			}
			else
			{
				throw parse_error_tpl(errors::unexpected_eof());
			}
		}


		template <typename t_delimiter, field_position t_field_position = field_position::middle_, typename t_range>
		requires t_range::is_contiguous
		constexpr inline bool parse(t_range &range, std::string_view &dst) const
		{
			return parse_ <t_delimiter, t_field_position>(range, dst);
		}


		template <typename t_delimiter, field_position t_field_position = field_position::middle_, typename t_range>
		requires t_range::is_contiguous
		constexpr inline bool parse(t_range &range, std::string &dst) const
		{
			return parse_ <t_delimiter, t_field_position>(range, dst);
		}
	};


	template <typename t_integer, bool t_is_signed = std::is_signed_v <t_integer>>
	struct integer
	{
		template <bool t_should_copy>
		using value_type = t_integer;
	
	
		template <typename t_delimiter, field_position t_field_position = field_position::middle_, typename t_range>
		LIBBIO_CONSTEXPR_WITH_GOTO inline bool parse(t_range &range, t_integer &val) const
		{
			bool is_negative{};
			val = 0;
		
			if constexpr (any(field_position::initial_ & t_field_position))
			{
				if (range.is_at_end())
					return false;
			
				if constexpr (t_is_signed)
					goto continue_parsing_1;
				else
					goto continue_parsing_2;
			}
	
			if (t_is_signed) // Can’t make this if constexpr b.c. we would like to jump to continue_parsing_1.
			{
				if (!range.is_at_end())
				{
				continue_parsing_1:
					auto const cc(*range.it);
					switch (cc)
					{
						case '-':
							if constexpr (!std::is_signed_v <t_integer>)
								throw parse_error_tpl(errors::unexpected_character('-'));
				
							is_negative = true;
							++range.it;
							break;
				
						case '+':
							++range.it;
							break;
				
						default:
							if (t_delimiter::matches(cc))
								throw parse_error_tpl(errors::unexpected_character(cc));
							goto continue_parsing_2;
					}
				}
			}

			while (!range.is_at_end())
			{
			continue_parsing_2:
				auto const cc(*range.it);
				if (t_delimiter::matches(cc))
				{
					if constexpr (t_is_signed)
					{
						if (is_negative)
							val *= -1;
					}
					
					++range.it;
					return true;
				}
				
				val *= 10;
				if (! ('0' <= cc && cc <= '9'))
					throw parse_error_tpl(errors::unexpected_character(cc));
				val += cc - '0';
				++range.it;
			}
			
			if constexpr (any(field_position::final_ & t_field_position))
				throw parse_error_tpl(errors::unexpected_eof());
			
			return false;
		}
	};
	
	
	template <typename t_floating_point>
	struct floating_point
	{
		template <bool t_should_copy>
		using value_type = t_floating_point;
		
		
		template <typename t_delimiter, field_position t_field_position = field_position::middle_, typename t_range>
		constexpr inline bool parse(t_range &range, t_floating_point &val) const
		{
			if constexpr (any(field_position::initial_ & t_field_position))
			{
				if (range.is_at_end())
					return false;
			}
			
			namespace x3 = boost::spirit::x3;
			auto ip(range.joining_iterator_pair());
			if (x3::parse(std::get <0>(ip), std::get <1>(ip), x3::real_parser <t_floating_point>{}, val))
			{
				if constexpr (any(field_position::final_ & t_field_position))
				{
					if (range.is_at_end())
						return true;
					else if (t_delimiter::matches(*range.it))
					{
						++range.it;
						return true;
					}
					else
					{
						throw parse_error_tpl(errors::unexpected_character(*range.it));
					}
				}
				else // Not final.
				{
					if (range.is_at_end())
						throw parse_error_tpl(errors::unexpected_eof());
					else if (!t_delimiter::matches(*range.it))
						throw parse_error_tpl(errors::unexpected_character(*range.it));
				
					// *it matches t_delimiter.
					++range.it;
					return true;
				}
			}
			else // Unable to parse.
			{
				if constexpr (any(field_position::final_ & t_field_position))
				{
					if (range.is_at_end())
						return false;
					else
						throw parse_error_tpl(errors::unexpected_character(*range.it));
				}
				else // Not final.
				{
					if (range.is_at_end())
						throw parse_error_tpl(errors::unexpected_eof());
					else 
						throw parse_error_tpl(errors::unexpected_character(*range.it));
				}
			}
	
		}
	};
	
	
	template <typename t_numeric>
	using numeric = std::conditional_t <
		std::is_floating_point_v <t_numeric>,
		floating_point <t_numeric>,
		integer <t_numeric>
	>;
} 

#endif

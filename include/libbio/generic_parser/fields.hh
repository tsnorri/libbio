/*
 * Copyright (c) 2022-2023 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_GENERIC_PARSER_FIELDS_HH
#define LIBBIO_GENERIC_PARSER_FIELDS_HH

#include <boost/spirit/home/x3.hpp>
#include <libbio/generic_parser/delimiter.hh>
#include <libbio/generic_parser/errors.hh>
#include <libbio/generic_parser/field_position.hh>
#include <libbio/generic_parser/filters.hh>
#include <libbio/generic_parser/iterators.hh>
#include <libbio/tuple/cat.hh>
#include <libbio/tuple/find.hh>
#include <libbio/tuple/map.hh>
#include <libbio/tuple/reusable_tuple.hh>
#include <libbio/tuple/unique.hh>
#include <libbio/tuple/utility.hh>
#include <string>
#include <string_view>
#include <type_traits>							// std::bool_constant, std::is_signed_v


// Clang 14 follows the standard in the sense that goto is not allowed in constexpr functions
// (even in if !consteval block).
#if defined(__GNUG__) && !defined(__clang__)
#	define LIBBIO_CONSTEXPR_WITH_GOTO	constexpr
#else
#	define LIBBIO_CONSTEXPR_WITH_GOTO
#endif


namespace libbio::parsing {
	
	template <typename t_tag, typename t_parser>
	struct tagged_parser
	{
		typedef t_parser	parser_type;
		typedef t_tag		tag_type;
	};
	
	
	struct empty_tag {};
	
	template <typename t_lhs, typename t_rhs>
	constexpr bool operator==(t_lhs const, t_rhs const)
	requires (std::is_base_of_v <empty_tag, t_lhs> && std::is_base_of_v <empty_tag, t_rhs>)
	{
		return std::is_same_v <t_lhs, t_rhs>;
	}
	
	
	template <typename t_type, bool = false>
	struct is_optional_repeating : public std::false_type {};
	
	template <typename t_type>
	struct is_optional_repeating <t_type, bool(t_type::is_optional_repeating::value)> : public std::bool_constant <t_type::is_optional_repeating::value> {};
	
	template <typename t_type>
	constexpr static inline bool is_optional_repeating_v = is_optional_repeating <t_type>::value;
	
	
	template <typename t_type, bool t_is_repeating = is_optional_repeating_v <t_type>>
	struct is_repeating : public std::bool_constant <t_is_repeating> {};
	
	template <typename t_type>
	struct is_repeating <t_type, bool(t_type::is_repeating::value)> : public std::bool_constant <t_type::is_repeating::value> {};
	
	template <typename t_type>
	constexpr static inline bool is_repeating_v = is_repeating <t_type>::value;
	
	
	template <typename t_type>
	struct is_alternative : public std::false_type {};
	
	template <typename... t_args>
	constexpr static inline auto const is_alternative_v{is_alternative <t_args...>::value};
	
	
	// Indicates multiple output options.
	template <typename... t_type>
	struct alternative
	{
		typedef std::tuple <t_type...>	tuple_type;
		
		constexpr static auto const max_size{[]() consteval -> std::size_t {
			static_assert(!(is_alternative_v <t_type> || ...));
			std::size_t retval{};
			return ((retval = std::max(retval, tuples::reusable_tuple_size_v <t_type>)), ...);
		}()};
		
		constexpr static auto const max_alignment{[]() consteval -> std::size_t {
			return tuples::visit_all_parameters <tuple_type>([]<typename... t_tuples>() consteval -> std::size_t {
				if constexpr (0 == sizeof...(t_tuples))
					return 1;
				else
				{
					static_assert(all(tuples::is_tuple_v <t_tuples>...));
					std::size_t retval{1};
					return ((retval = std::max(retval, tuples::visit_all_parameters <t_tuples>([]<typename... t_type_>() consteval -> std::size_t {
						if constexpr (0 == sizeof...(t_type_))
							return 1;
						else
						{
							static_assert(!(is_alternative_v <t_type_> || ...));
							std::size_t retval{1};
							return ((retval = std::max(retval, alignof(t_type_))), ...);
						}
					}))), ...);
				}
			});
		}()};
	};
	
	
	template <typename... t_args>
	struct is_alternative <alternative <t_args...>> : public std::true_type {};
	
	
	template <typename t_type>
	struct to_alternative {};
	
	template <typename... t_args>
	struct to_alternative <std::tuple <t_args...>>
	{
		typedef alternative <t_args...>	type;
	};
	
	template <typename t_type>
	using to_alternative_t = typename to_alternative <t_type>::type;
	
	
	struct boolean_wrapper
	{
		bool value{};
		
		constexpr /* implicit */ operator bool() const noexcept { return value; }
		
		constexpr boolean_wrapper &operator=(char const cc)
		{
			// Handle parsing here to keep fields::character_like relatively simple.
			switch (cc)
			{
				case 'T':
				case 'Y':
					value = true;
					break;
				
				case 'F':
				case 'N':
					value = false;
					break;
				
				default:
					throw parse_error_tpl(errors::unexpected_character(cc));
			}
			
			return *this;
		}
	};


	struct parsing_result
	{
		delimiter_index_type	matched_delimiter_index{INVALID_DELIMITER_INDEX};
		bool					did_succeed{};

		parsing_result() = default;

		parsing_result(bool const did_succeed_, delimiter_index_type const matched_delimiter_index_):
			matched_delimiter_index(matched_delimiter_index_),
			did_succeed(did_succeed_)
		{
		}

		/* implicit */ parsing_result(delimiter_index_type const matched_delimiter_index_):
			parsing_result(true, matched_delimiter_index_)
		{
		}

		operator bool() const { return did_succeed; }
	};
}


namespace libbio::parsing::detail {
	
	template <typename t_tag_>
	struct matches_tag
	{
		template <typename>
		struct with {};
		
		template <typename t_other_tag, typename t_parser>
		struct with <tagged_parser <t_other_tag, t_parser>> :
			public std::bool_constant <std::is_same_v <t_tag_, t_other_tag>> {};
	};
	
	
	template <typename t_type, typename t_value_type, t_value_type t_default, bool t_is_void = std::is_void_v <t_type>>
	struct value_if_not_void
	{
		constexpr static inline t_value_type const value{t_default};
	};
	
	template <typename t_type, typename t_value_type, t_value_type t_default>
	struct value_if_not_void <t_type, t_value_type, t_default, false>
	{
		constexpr static inline t_value_type const value{t_type::value};
	};
	
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
		LIBBIO_CONSTEXPR_WITH_GOTO inline parsing_result parse(t_range &range) const
		{
			if constexpr (any(field_position::initial_ & t_field_position))
			{
				if (range.is_at_end())
					return {};
				
				goto continue_parsing;
			}
	
			while (!range.is_at_end())
			{
			continue_parsing:
				if (auto const idx{t_delimiter::matching_index(*range.it)}; t_delimiter::size() != idx)
				{
					++range.it;
					return {idx};
				}
				
				++range.it;
			}
		
			if constexpr (any(field_position::final_ & t_field_position))
			{
				// t_delimiter not matched.
				throw parse_error_tpl(errors::unexpected_eof());
			}
		
			return {};
		}
	};
	
	
	template <
		typename t_delimiter,
		typename t_character_filter = filters::no_op,
		field_position t_field_position = field_position::middle_,
		typename /* ignored */,
		typename t_range,
		typename t_dst
	>
	requires (!std::remove_cvref_t <t_range>::is_contiguous)
	LIBBIO_CONSTEXPR_WITH_GOTO inline parsing_result parse_sequential(t_range &&range, t_dst &&dst)
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
			auto const cc(*range.it);
			if (auto const idx{t_delimiter::matching_index(cc)}; t_delimiter::size() != idx)
			{
				++range.it;
				return {idx};
			}
			else if (!t_character_filter::check(cc))
			{
				throw parse_error_tpl(errors::unexpected_character(cc));
			}
	
			dst.handle_character(cc);
			++range.it;
		}
		
		// t_delimiter not matched.
		if constexpr (any(field_position::final_ & t_field_position))
			return {INVALID_DELIMITER_INDEX};
		else
			throw parse_error_tpl(errors::unexpected_eof());
	}
	
	
	template <
		typename t_delimiter,
		typename t_character_filter = filters::no_op,
		field_position t_field_position = field_position::middle_,
		typename t_assigned,
		typename t_range,
		typename t_dst
	>
	requires std::remove_cvref_t <t_range>::is_contiguous
	LIBBIO_CONSTEXPR_WITH_GOTO inline parsing_result parse_sequential(t_range &&range, t_dst &&dst)
	{
		auto begin(range.it); // Copy.

		if constexpr (any(field_position::initial_ & t_field_position))
		{
			if (range.is_at_end())
				return {};
			goto continue_parsing;
		}

		while (!range.is_at_end())
		{
		continue_parsing:
			auto const cc(*range.it);
			if (auto const idx{t_delimiter::matching_index(cc)}; t_delimiter::size() != idx)
			{
				dst = t_assigned(begin, range.it);
				++range.it;
				return {idx};
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
			dst = t_assigned(begin, range.it);
			return {INVALID_DELIMITER_INDEX};
		}
		else
		{
			throw parse_error_tpl(errors::unexpected_eof());
		}
	}


	template <typename t_character_filter = filters::no_op, typename t_should_always_copy = void>
	struct text
	{
		template <bool t_should_copy>
		using value_type = typename detail::text_value_type_tpl <
			parsing::detail::value_if_not_void <t_should_always_copy, bool, t_should_copy>::value
		>::type;


		template <typename t_delimiter, field_position t_field_position = field_position::middle_, typename t_range, typename t_dst>
		constexpr inline parsing_result parse(t_range &range, t_dst &dst) const
		{
			struct helper
			{
				t_dst &dst;
				
				void clear() { dst.clear(); }
				void handle_character(char const cc) { dst.push_back(cc); }
			};
			
			helper hh{dst};
			return parse_sequential <t_delimiter, t_character_filter, t_field_position, std::string_view>(range, hh);
		}
	};
	
	
	template <bool t_should_always_copy, typename t_character_filter = filters::no_op>
	using text_ = text <t_character_filter, std::bool_constant <t_should_always_copy>>;
	
	
	template <typename t_char, typename t_value = std::vector <t_char>, typename t_character_filter = filters::no_op>
	struct character_sequence
	{
		template <bool t_should_copy>
		using value_type = t_value;
		
		template <typename t_delimiter, field_position t_field_position = field_position::middle_, typename t_range, typename t_dst>
		constexpr inline parsing_result parse(t_range &range, t_dst &dst) const
		{
			struct helper
			{
				t_dst &dst;
				
				void clear() { dst.clear(); }
				void handle_character(char const cc) { dst.emplace_back(cc); }
			};
			
			helper hh{dst};
			return parse_sequential <t_delimiter, t_character_filter, t_field_position, t_value>(range, hh); // FIXME: replace t_value with some other parameter?
		}
	};
	
	
	template <typename t_value_type>
	struct character_like
	{
		template <bool t_should_copy>
		using value_type = t_value_type;
		
		
		template <field_position t_field_position = field_position::middle_, typename t_range>
		constexpr inline bool parse_value(t_range &range, t_value_type &val) const
		{
			if constexpr (any(field_position::initial_ & t_field_position))
			{
				if (range.is_at_end())
					return true;
			}
			else
			{
				if (range.is_at_end())
					throw parse_error_tpl(errors::unexpected_eof());
			}
			
			val = *range.it;
			++range.it;
			return true;
		}
		
		
		template <typename t_delimiter, field_position t_field_position = field_position::middle_, typename t_range>
		constexpr inline parsing_result parse(t_range &range, t_value_type &val) const
		{
			if (!parse_value(range, val))
				return {};
			
			if constexpr (any(field_position::final_ & t_field_position))
			{
				if (range.is_at_end())
					return {INVALID_DELIMITER_INDEX};
			}
			else
			{
				if (range.is_at_end())
					throw parse_error_tpl(errors::unexpected_eof());
			}
			
			auto const cc(*range.it);
			if (auto const idx{t_delimiter::matching_index(cc)}; t_delimiter::size() != idx)
			{
				++range.it;
				return {idx};
			}
				
			// Characters left but t_delimiter not matched.
			throw parse_error_tpl(errors::unexpected_character(cc));
		}
	};
	
	
	typedef character_like <char>				character;
	typedef character_like <boolean_wrapper>	boolean;
	
	
	template <typename t_integer, bool t_is_signed = std::is_signed_v <t_integer>>
	struct integer
	{
		template <bool t_should_copy>
		using value_type = t_integer;
		
		
		template <field_position t_field_position = field_position::middle_, typename t_range>
		LIBBIO_CONSTEXPR_WITH_GOTO inline bool parse_value(t_range &range, t_integer &val) const
		{
			bool did_parse{};
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
							goto continue_parsing_2;
					}
				}
			}
			
			while (!range.is_at_end())
			{
			continue_parsing_2:
				auto const cc(*range.it);
				
				if ('0' <= cc && cc <= '9')
				{
					did_parse = true;
					val *= 10;
					val += cc - '0';
				}
				else
				{
					if constexpr (t_is_signed)
					{
						if (is_negative)
							val *= -1;
					}
					break;
				}
				
				++range.it;
			}
			
			if (!did_parse)
			{
				if (range.is_at_end())
					throw parse_error_tpl(errors::unexpected_eof());
				else
					throw parse_error_tpl(errors::unexpected_character(*range.it));
			}
			
			return true;
		}
		
		
		template <typename t_delimiter, field_position t_field_position = field_position::middle_, typename t_range>
		LIBBIO_CONSTEXPR_WITH_GOTO inline parsing_result parse(t_range &range, t_integer &val) const
		{
			bool is_negative{};
			val = 0;
		
			if constexpr (any(field_position::initial_ & t_field_position))
			{
				if (range.is_at_end())
					return {};
			
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
				if (auto const idx{t_delimiter::matching_index(cc)}; t_delimiter::size() != idx)
				{
					if constexpr (t_is_signed)
					{
						if (is_negative)
							val *= -1;
					}
					
					++range.it;
					return {idx};
				}
				
				val *= 10;
				if (! ('0' <= cc && cc <= '9'))
					throw parse_error_tpl(errors::unexpected_character(cc));
				val += cc - '0';
				++range.it;
			}
			
			if constexpr (any(field_position::final_ & t_field_position))
				throw parse_error_tpl(errors::unexpected_eof());
			
			return {};
		}
	};
	
	
	template <typename t_floating_point>
	struct floating_point
	{
		template <bool t_should_copy>
		using value_type = t_floating_point;
		
		
		template <typename t_delimiter, field_position t_field_position = field_position::middle_, typename t_range>
		constexpr inline parsing_result parse(t_range &range, t_floating_point &val) const
		{
			if constexpr (any(field_position::initial_ & t_field_position))
			{
				if (range.is_at_end())
					return {};
			}
			
			namespace x3 = boost::spirit::x3;
			auto ip(range.joining_iterator_pair());
			if (x3::parse(std::get <0>(ip), std::get <1>(ip), x3::real_parser <t_floating_point>{}, val))
			{
				if constexpr (any(field_position::final_ & t_field_position))
				{
					if (range.is_at_end())
						return {INVALID_DELIMITER_INDEX};
					else if (auto const idx{t_delimiter::matching_index(*range.it)}; t_delimiter::size() != idx)
					{
						++range.it;
						return {idx};
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

					delimiter_index_type const idx{t_delimiter::matching_index(*range.it)};
					if (t_delimiter::size() == idx)
						throw parse_error_tpl(errors::unexpected_character(*range.it));
				
					// *it matches t_delimiter.
					++range.it;
					return {idx};
				}
			}
			else // Unable to parse.
			{
				if constexpr (any(field_position::final_ & t_field_position))
				{
					if (range.is_at_end())
						return {};
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
	
	
	template <typename t_base, typename... t_tagged_parser>
	struct conditional : public t_base
	{
	public:
		typedef std::tuple <t_tagged_parser...> tagged_parsers_type;
		
	private:
		// Each parser may have either std::tuple <...> or alternative <std::tuple <...>, ...> as its record type.
		template <
			typename t_acc,
			typename t_tagged_parser_,
			bool t_parsed_type_is_alternative = t_tagged_parser_::parser_type::parsed_type_is_alternative
		>
		struct cat
		{
			// t_parsed_type_is_alternative == false.
			// Append the tuple to t_acc.
			
			typedef typename t_tagged_parser_::tag_type		tag_type;
			typedef typename t_tagged_parser_::parser_type	parser_type;
			typedef typename parser_type::record_type		record_type; // std::tuple
			
			typedef tuples::append_t <
				t_acc,
				typename tuples::prepend_t <tag_type, record_type>
			> type;
		};
		
		template <typename t_acc, typename t_tagged_parser_>
		struct cat <t_acc, t_tagged_parser_, true>
		{
			typedef typename t_tagged_parser_::tag_type		tag_type;
			typedef typename t_tagged_parser_::parser_type	parser_type;
			typedef typename parser_type::record_type		record_type; // alternative
			
			template <typename t_tuple>
			using prepend_t = tuples::prepend_t <tag_type, t_tuple>;
			
			typedef tuples::cat_t <
				t_acc,
				tuples::map_t <
					typename record_type::tuple_type,
					prepend_t
				>
			> type;
		};
		
		template <typename t_acc, typename t_item>
		using cat_t = typename cat <t_acc, t_item>::type;
		
		typedef tuples::unique_t <
			tuples::foldl_t <
				cat_t,	// Handle alternative.
				tuples::empty,
				tagged_parsers_type
			>
		> all_record_types;
		
		
		template <
			typename t_delimiter,
			field_position t_field_position,
			typename t_stack,
			typename t_range,
			typename t_dst,
			typename t_buffer,
			typename t_parse_cb
		>
		class callback_target
		{
			friend struct conditional;
			
			t_range		&m_range;
			t_dst		&m_dst;
			t_buffer	&m_buffer;
			t_parse_cb	&m_parse_cb;
			
			callback_target(t_range &range, t_dst &dst, t_buffer &buffer, t_parse_cb &parse_cb):
				m_range(range),
				m_dst(dst),
				m_buffer(buffer),
				m_parse_cb(parse_cb)
			{
			}
			
		public:
			constexpr t_range &range() { return m_range; }
			
			constexpr inline void read_delimiter()
			{
				++m_range.it;
				if (m_range.is_at_end())
					throw parse_error_tpl(errors::unexpected_eof());
			
				if (t_delimiter::matches(*m_range.it))
					++m_range.it;
				else
					throw parse_error_tpl(errors::unexpected_character(*m_range.it));
			}
			
			template <typename t_tag>
			constexpr inline bool continue_parsing(t_tag &&tag)
			{
				// Find a parser that matches t_tag.
				typedef tuples::find_if <
					tagged_parsers_type,
					libbio::parsing::detail::matches_tag <t_tag>::template with
				>																find_result_type;
				typedef typename find_result_type::first_match_type				tagged_parser_type;
				static_assert(!std::is_same_v <tuples::not_found, tagged_parser_type>, "Parser not found for the given tag");
				typedef typename tagged_parser_type::parser_type				parser_type;
			
				auto &dst(m_dst.template append <std::remove_cvref_t <t_tag>>(std::forward <t_tag>(tag)));
				return parser_type::template parse_alternatives <0, t_stack>(m_range, dst, m_buffer, m_parse_cb);
			}
		};
		
	public:
		template <bool t_should_copy>
		using value_type = std::conditional_t <
			1 == std::tuple_size_v <all_record_types>,
			std::tuple_element_t <1, all_record_types>,
			to_alternative_t <all_record_types>
		>;
	
		template <
			typename t_delimiter,
			field_position t_field_position = field_position::middle_,
			typename t_stack,
			typename t_range,
			typename t_dst,
			typename t_buffer,
			typename t_parse_cb
		>
		constexpr inline parsing_result parse(t_range &range, t_dst &dst, t_buffer &buffer, t_parse_cb &parse_cb) const
		{
			if constexpr (any(field_position::initial_ & t_field_position))
			{
				if (range.is_at_end())
					return {};
			}
			else
			{
				if (range.is_at_end())
					throw parse_error_tpl(errors::unexpected_eof());
			}
			
			typedef callback_target <t_delimiter, t_field_position, t_stack, t_range, t_dst, t_buffer, t_parse_cb> target_type;
			return t_base::parse(target_type(range, dst, buffer, parse_cb));
		}
	};
} 

#endif

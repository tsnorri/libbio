/*
 * Copyright (c) 2022 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <catch2/catch.hpp>
#include <libbio/file_handling.hh>
#include <libbio/fmap.hh>
#include <libbio/generic_parser.hh>
#include <rapidcheck.h>
#include <rapidcheck/catch.h>		// rc::prop
#include <tuple>

namespace lb	= libbio;
namespace lbp	= libbio::parsing;


namespace {
	
	// For filtering characters in generated strings.
	struct string
	{
		std::string	value;
	};
	
	
	std::ostream &operator<<(std::ostream &os, string const &str)
	{
		os << str.value;
		return os;
	}
	
	
	template <typename t_type>
	struct type_mapping
	{
		typedef t_type type;
	};
	
	template <>
	struct type_mapping <std::string>
	{
		typedef string type;
	};
	
	template <typename t_type>
	using mapped_type = typename type_mapping <t_type>::type;
	
	
	template <typename t_type>
	auto const &input_value(t_type const &value)
	{
		return value;
	}
	
	template <>
	auto const &input_value(string const &str)
	{
		return str.value;
	}
	
	
	template <typename t_fields>
	struct generated_input_types
	{
		template <std::size_t t_i>
		auto helper() const
		{
			if constexpr (t_i == std::tuple_size_v <t_fields>)
				return std::tuple{};
			else
			{
				typedef std::tuple_element_t <t_i, t_fields>			field_type;
				typedef typename field_type::template value_type <true>	value_type;
				if constexpr (std::is_same_v <void, value_type>)
					return std::tuple_cat(std::tuple <mapped_type <std::string>>{}, helper <1 + t_i>());
				else
					return std::tuple_cat(std::tuple <mapped_type <value_type>>{}, helper <1 + t_i>());
			}
		}
		
		typedef std::invoke_result_t <decltype(&generated_input_types::helper <0>), generated_input_types> type;
	};
	
	
	template <typename... t_args>
	struct parser_type
	{
		typedef lbp::parser <
			lbp::traits::delimited <lbp::delimiter <'\t'>, lbp::delimiter <'\n'>>,
			t_args...
		> type;
	};
	
	
	template <typename... t_fields>
	struct test_case
	{
		typedef std::tuple <t_fields...>							field_tuple;
		typedef typename generated_input_types <field_tuple>::type	input_tuple;
		typedef typename parser_type <t_fields...>::type			parser_type;
		typedef typename parser_type::record_type					record_type;
	};
	
	
	template <char t_sep, std::size_t t_i = 0, typename t_tuple>
	void output_tuple_as_delimited(std::ostream &os, t_tuple const &tup)
	{
		if constexpr (t_i < std::tuple_size_v <t_tuple>)
		{
			using std::get;
			using libbio::tuples::get;
			if constexpr (t_i == 0)
			{
				os << get <0>(tup);
				output_tuple_as_delimited <t_sep, 1 + t_i>(os, tup);
			}
			else
			{
				os << t_sep << get <t_i>(tup);
				output_tuple_as_delimited <t_sep, 1 + t_i>(os, tup);
			}
		}
	}
}


namespace rc {
	
	template <>
	struct Arbitrary <string>
	{
		static Gen <string> arbitrary()
		{
			return gen::map(gen::string <std::string>(), [](std::string &&str) -> string {
				// Remove tabulators and newlines.
				std::erase_if(str, [](auto const cc){ return '\t' == cc || '\n' == cc; });
				return {std::move(str)};
			});
		}
	};
}


TEMPLATE_TEST_CASE(
	"generic_parser with arbitrary input",
	"[template][generic_parser]",
	(test_case <lbp::fields::text <>>),
	(test_case <lbp::fields::numeric <std::int32_t>>),
	(test_case <lbp::fields::numeric <std::uint32_t>>),
	(test_case <
		lbp::fields::text <>,
		lbp::fields::text <>
	>),
	(test_case <
		lbp::fields::numeric <std::int32_t>,
		lbp::fields::text <>
	>),
	(test_case <
		lbp::fields::text <>,
		lbp::fields::numeric <std::int32_t>,
		lbp::fields::text <>
	>),
	(test_case <
		lbp::fields::numeric <std::int32_t>,
		lbp::fields::text <>,
		lbp::fields::numeric <std::int32_t>
	>),
	(test_case <
		lbp::fields::text <>,
		lbp::fields::skip,
		lbp::fields::text <>
	>)
)
{
	typedef typename TestType::input_tuple input_tuple;
	typedef typename TestType::parser_type parser_type;
	typedef typename TestType::record_type record_type;
	
	rc::prop(
		"generic_parser can parse valid inputs",
		[](std::vector <input_tuple> const &input){
			std::stringstream stream;
			std::vector <record_type> expected_results;
			
			for (auto const &tup : input)
			{
				output_tuple_as_delimited <'\t'>(stream, tup);
				stream << '\n';
				expected_results.emplace_back(
					lb::fmap(typename parser_type::included_field_indices{},
					[&tup](auto const idx){
						return input_value(std::get <decltype(idx)::value>(tup));
					})
				);
			}
			
			stream.seekg(0);
			
			parser_type parser;
			std::istreambuf_iterator it(stream.rdbuf());
			std::istreambuf_iterator <decltype(it)::value_type> const sentinel;
			
			std::vector <record_type> actual_results;
			while (true)
			{
				record_type rec;
				if (!parser.parse(it, sentinel, rec))
					break;
				
				actual_results.emplace_back(std::move(rec));
			}
			
			if (expected_results != actual_results)
			{
				std::stringstream os;
				os << "Failed.\n";
				os << "Expected: ";
				for (auto const &tup : expected_results)
				{
					output_tuple_as_delimited <'\t'>(os, tup);
					os << '\n';
				}
				os << '\n';
				os << "Actual:   ";
				for (auto const &tup : actual_results)
				{
					output_tuple_as_delimited <'\t'>(os, tup);
					os << '\n';
				}
				os  << '\n';
				os << "Stream: “" << stream.str() << "”\n";
				RC_FAIL(os.str());
			}
			
			return true;
		}
	);
}


namespace {
	
	struct text_tag : public lbp::empty_tag {};
	struct integer_tag : public lbp::empty_tag {};
	
	std::ostream &operator<<(std::ostream &os, text_tag const) { os << "(text_tag)"; return os; }
	std::ostream &operator<<(std::ostream &os, integer_tag const) { os << "(integer_tag)"; return os; }
	
	struct conditional_field
	{
		template <typename t_caller>
		bool parse(t_caller &&caller) const
		{
			auto &range(caller.range());
			switch (*range.it)
			{
				case 'C':
					caller.read_delimiter();
					return caller.continue_parsing(text_tag{});
					
				case 'I':
					caller.read_delimiter();
					return caller.continue_parsing(integer_tag{});
					
				default:
					throw lbp::parse_error_tpl(lbp::errors::unexpected_character(*range.it));
			}
		}
	};
}


SCENARIO("libbio::parser::conditional_field works with simple input")
{
	GIVEN("a simple input file")
	{
		lb::file_istream stream;
		lb::open_file_for_reading("test.tsv", stream);
		
		WHEN("the file is parsed")
		{
			typedef lbp::traits::delimited <lbp::delimiter <'\t'>>							parser_traits;
			typedef lbp::traits::delimited <lbp::delimiter <'\t'>, lbp::delimiter <'\n'>>	conditional_parser_traits;
			typedef lbp::parser <
				parser_traits,
				lbp::fields::character,
				lbp::fields::make_conditional <
					conditional_field,
					conditional_parser_traits,
					lbp::fields::option <text_tag, lbp::fields::text <>>,
					lbp::fields::option <integer_tag, lbp::fields::integer <std::int32_t>>
				>
			> parser_type;
			
			std::istreambuf_iterator it(stream);
			std::istreambuf_iterator <char> const sentinel;
			auto range(lbp::make_range(it, sentinel));
			
			parser_type parser;
			parser_type::record_type rec;
			parser_type::buffer_type buffer;
			
			std::size_t row_idx{};
			while (true)
			{
				auto const res(parser.parse(range, rec, buffer, [&row_idx](auto const &rec){
					switch (row_idx)
					{
						case 0:
							CHECK(rec == std::make_tuple('a', text_tag{}, std::string("asdf")));
							break;
							
						case 1:
							CHECK(rec == std::make_tuple('b', integer_tag{}, 123));
							break;
						
						default:
							FAIL_CHECK("Parsed more rows than expected.");
							break;
					}
					
					++row_idx;
				}));
				
				if (!res)
					break;
			}
			CHECK(2 == row_idx);
		}
	}
}

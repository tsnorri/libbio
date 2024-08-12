/*
 * Copyright (c) 2022-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <boost/format.hpp>
#include <cstring>
#include <libbio/subprocess.hh>


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"


%% machine subprocess_argument_parser;
%% write data;


namespace lb	= libbio;


namespace libbio {
	
	// Parse plain arguments that do not contain spaces and
	// double quoted arguments that may contain spaces and escaped double quotes ("").
	std::vector <std::string> parse_command_arguments(char const * const args)
	{
		if (!args)
			return {};

		auto const *p(args);
		auto const *pe(p + ::strlen(args));
		auto const *eof(pe);
		auto const *arg_begin(p);
		int cs(0);
		bool should_append{};
		
		std::vector <std::string> retval;
		
		%%{
			machine subprocess_argument_parser;
			
			action begin_arg {
				arg_begin = fpc;
				should_append = false;
			}
			
			action end_plain {
				std::string_view const sv(arg_begin, fpc);
				retval.emplace_back(sv);
			}
			
			action end_escaped_quote {
				// end_quoted gets executed when the first quote is seen.
				if (should_append)
					retval.back() += '"';
				else
					retval.emplace_back("\"");
				should_append = true;
				arg_begin = fpc;
			}
			
			action end_quoted {
				std::string_view const sv(arg_begin, fpc);
				if (should_append)
					retval.back() += sv;
				else
					retval.emplace_back(sv);
				should_append = true; // In case we processed an escaping quote.
				arg_begin = fpc + 1;
			}
			
			action error {
				auto msg(boost::str(boost::format("Unepected character %d at position %zu") % (+fc) % (fpc - args)));
				throw std::invalid_argument(std::move(msg));
			}
			
			quote			= ["];
			quoted_char		= print - quote;
			plain_char		= graph - quote;
			escaped_quote	= ('""') %(end_escaped_quote);
			quoted_text		= (quoted_char | escaped_quote)+;
			quoted_argument = quote quoted_text >(begin_arg) %(end_quoted) quote;
			plain_argument	= (plain_char+) >(begin_arg) %(end_plain);
			argument		= plain_argument | quoted_argument;
			argument_string	= (argument (space+ argument)*)?;
			main			:= argument_string $err(error);
			
			write init;
			write exec;
		}%%
		
		return retval;
	}
}

#pragma GCC diagnostic pop

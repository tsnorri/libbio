/*
 * Copyright (c) 2023 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <libbio/fasta_reader.hh>
#include <libbio/file_handling.hh>
#include <libbio/rapidcheck/markov_chain.hh>
#include <range/v3/view/reverse.hpp>
#include <range/v3/view/transform.hpp>
#include <thread>
#include "rapidcheck_test_driver.hh"

namespace ios	= boost::iostreams;
namespace lb	= libbio;
namespace mcs	= libbio::markov_chains;
namespace rsv	= ranges::views;


namespace {
	
	static std::string const sequence_characters{"ACGTUMRWSYKVHDBN-acgtumrwsykvhdbn"};
	static std::string const comment_characters{"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_; "};
	static std::string const header_characters{"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_"};
	
	
	struct line_base
	{
		std::string content;
	};
	
	struct header_line : public line_base
	{
		std::vector <std::string> extra;
	};
	
	struct sequence_line : public line_base {};
	struct comment_line : public line_base {};
	struct initial_comment_line : public comment_line {};
	
	struct initial_state
	{
		sequence_line	final_sequence_line{};
	};
	
	
	std::ostream &operator<<(std::ostream &os, line_base const &ll)
	{
		os << ll.content;
		return os;
	}
	
	
	std::ostream &operator<<(std::ostream &os, header_line const &ll)
	{
		os << ll.content;
		for (auto const &extra : ll.extra)
			os << ' ' << extra;
		return os;
	}
	
	
	std::ostream &operator<<(std::ostream &os, initial_state const &ll)
	{
		os << ll.final_sequence_line.content;
		return os;
	}
	
	
	struct fasta_line
	{
		enum class line_type : std::uint8_t
		{
			no_type		= 0,
			comment		= 0x1,
			header		= 0x2,
			sequence	= 0x4
		};
		
		typedef std::underlying_type_t <line_type> line_type_;
		
		line_type					type{line_type::no_type};
		std::string					content;
		std::vector <std::string>	extra;
		
		/* implicit */ fasta_line(initial_state &&ll):
			type(line_type::sequence),
			content(std::move(ll.final_sequence_line.content))
		{
		}
		
		/* implicit */ fasta_line(initial_comment_line &&ll):
			type(line_type::comment),
			content(std::move(ll.content))
		{
		}
		
		/* implicit */ fasta_line(header_line &&ll):
			type(line_type::header),
			content(std::move(ll.content)),
			extra(std::move(ll.extra))
		{
		}
		
		/* implicit */ fasta_line(sequence_line &&ll):
			type(line_type::sequence),
			content(std::move(ll.content))
		{
		}
		
		/* implicit */ fasta_line(comment_line &&ll):
			type(line_type::comment),
			content(std::move(ll.content))
		{
		}
		
		decltype(auto) to_tuple() const { return std::tie(type, content, extra); }
		bool operator==(fasta_line const &other) const { return to_tuple() == other.to_tuple(); }
	};
	
	
	std::ostream &operator<<(std::ostream &os, fasta_line const &line)
	{
		switch (line.type)
		{
			case fasta_line::line_type::comment:
				os << ';' << line.content << '\n';
				break;
				
			case fasta_line::line_type::header:
				os << '>' << line.content;
				for (auto const &ee : line.extra)
					os << '\t' << ee;
				os << '\n';
				break;
				
			case fasta_line::line_type::sequence:
				os << line.content << '\n';
				break;
			
			default:
				RC_FAIL("Unexpected line type");
		}
		return os;
	}
	
	
	struct fasta_input_with_sequence_headers
	{
		std::vector <fasta_line> lines;
	};
	
	
	std::ostream &operator<<(std::ostream &os, fasta_input_with_sequence_headers const &input)
	{
		for (auto const &line : input.lines)
			os << line;
		return os;
	}
	
	
	typedef mcs::chain <
		fasta_line,
		initial_state,
		mcs::transition_list <
			mcs::transition <initial_state,			initial_comment_line,	0.5>,
			mcs::transition <initial_state,			header_line,			0.5>,
			mcs::transition <initial_comment_line,	initial_comment_line,	0.25>,
			mcs::transition <initial_comment_line,	header_line,			0.75>,
			mcs::transition <header_line,			sequence_line,			1.0>,
			mcs::transition <sequence_line,			sequence_line,			0.5>,
			mcs::transition <sequence_line,			comment_line,			0.2>,
			mcs::transition <sequence_line,			header_line,			0.3>,
			mcs::transition <comment_line,			header_line,			0.75>,
			mcs::transition <comment_line,			comment_line,			0.25>
		>
	> test_input_markov_chain_type;
}
	
	
namespace rc {
	
	template <>
	struct Arbitrary <header_line>
	{
		static Gen <header_line> arbitrary()
		{
			return gen::construct <header_line>(
				gen::nonEmpty(
					gen::container <std::string>(gen::elementOf(header_characters))
				),
				gen::container <std::vector <std::string>>(
					gen::nonEmpty(
						gen::container <std::string>(gen::elementOf(header_characters))
					)
				)
			);
		}
	};
	
	
	template <>
	struct Arbitrary <sequence_line>
	{
		static Gen <sequence_line> arbitrary()
		{
			return gen::construct <sequence_line>(
				gen::nonEmpty(
					gen::container <std::string>(gen::elementOf(sequence_characters))
				)
			);
		}
	};
	
	
	template <>
	struct Arbitrary <comment_line>
	{
		static Gen <comment_line> arbitrary()
		{
			return gen::construct <comment_line>(
				gen::container <std::string>(gen::elementOf(comment_characters))
			);
		}
	};
	
	
	template <>
	struct Arbitrary <initial_comment_line>
	{
		static Gen <initial_comment_line> arbitrary()
		{
			return gen::construct <initial_comment_line>(
				gen::container <std::string>(gen::elementOf(comment_characters))
			);
		}
	};
	
	
	template <>
	struct Arbitrary <initial_state>
	{
		static Gen <initial_state> arbitrary()
		{
			return gen::construct <initial_state>(
				gen::arbitrary <sequence_line>()
			);
		}
	};
	
	
	template <>
	struct Arbitrary <fasta_input_with_sequence_headers>
	{
		static Gen <fasta_input_with_sequence_headers> arbitrary()
		{
			return gen::map(
				gen::arbitrary <test_input_markov_chain_type>(),
				[](auto &&chain) -> fasta_input_with_sequence_headers {
					if (chain.values.empty())
						return {};
					
					RC_ASSERT(fasta_line::line_type::sequence == chain.values.front().type);
					
					auto const has_header(chain.values.end() != std::find_if(
						chain.values.begin(),
						chain.values.end(),
						[](auto const &line){ return fasta_line::line_type::header == line.type; }
					));
					
					if (has_header)
					{
						// Move the final sequence line to the end of the vector.
						std::rotate(chain.values.begin(), chain.values.begin() + 1, chain.values.end());
					}
					else
					{
						// Remove non-sequence lines.
						chain.values.erase(
							std::remove_if(
								chain.values.begin(), 
								chain.values.end(),
								[](auto const &line) { return fasta_line::line_type::sequence != line.type; }
							),
							chain.values.end()
						);
					}
					return fasta_input_with_sequence_headers{std::move(chain.values)};
				}
			);
		}
	};
}


namespace {
	
	struct fasta_reader_delegate final : public lb::fasta_reader_delegate
	{
		std::vector <fasta_line>	parsed_lines;
		std::string					current_sequence_line;
		
		bool handle_identifier(lb::fasta_reader_base &reader, std::string_view const &identifier, std::vector <std::string_view> const &extra_fields) override
		{
			std::vector <std::string> extra_fields_;
			extra_fields_.reserve(extra_fields.size());
			for (auto const &extra : extra_fields)
				extra_fields_.emplace_back(extra);
			
			parsed_lines.emplace_back(fasta_line{header_line{
				{std::string{identifier}},
				std::move(extra_fields_)
			}});
			return true;
		}
		
		bool handle_sequence_chunk(lb::fasta_reader_base &reader, std::string_view const &sv, bool has_newline) override
		{
			current_sequence_line += sv;
			if (has_newline)
			{
				parsed_lines.emplace_back(fasta_line{sequence_line{std::move(current_sequence_line)}});
				current_sequence_line.clear();
			}
			return true;
		}
		
		bool handle_sequence_end(lb::fasta_reader_base &reader) override
		{
			return true;
		}
		
		bool handle_comment_line(lb::fasta_reader_base &reader, std::string_view const &sv) override
		{
			parsed_lines.emplace_back(fasta_line{comment_line{std::string{sv}}});
			return true;
		}
	};
}


TEST_CASE(
	"fasta_reader works with arbitrary input",
	"[fasta_reader]"
)
{
	return lb::rc_check(
		"fasta_reader works with arbitrary input",
		[](fasta_input_with_sequence_headers const &input){
			
			// Report the line types in the current input.
			fasta_line::line_type_ input_line_types_mask{};
			for (auto const &line : input.lines)
				input_line_types_mask |= std::to_underlying(line.type);
			RC_TAG(input_line_types_mask);
			
			std::array const blocksizes{0, 16, 64, 128, 256, 512, 1024, 2048};
			for (auto const blocksize : blocksizes)
			{
				// Write the generated input to a pipe (to get a pair of file descriptors) and parse.
				int fds[2]{};
				
				{
					auto const res(::pipe(fds));
					RC_ASSERT(-1 != res);
				}
				
				lb::file_handle read_handle(fds[0]);
				lb::file_handle write_handle(fds[1]);
				
				{
					std::thread write_thread([&, write_handle = std::move(write_handle)](){
						lb::file_ostream write_stream;
						write_stream.open(write_handle.get(), ios::never_close_handle);
						write_stream.exceptions(std::ostream::badbit);
					
						write_stream << input;
					});
					write_thread.detach();
				}
				
				lb::fasta_reader reader;
				fasta_reader_delegate delegate;
				if (0 == blocksize)
					reader.parse(read_handle, delegate);
				else
					reader.parse(read_handle, delegate, blocksize);
				
				RC_ASSERT(input.lines == delegate.parsed_lines);
			}
			
			return true;
		}
	);
}

/*
 * Copyright (c) 2023-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <array>
#include <cstdint>
#include <libbio/fasta_reader.hh>
#include <libbio/file_handle.hh>
#include <libbio/file_handling.hh>
#include <libbio/markov_chain.hh>
#include <libbio/rapidcheck_test_driver.hh>
#include <libbio/rapidcheck/markov_chain.hh>
#include <ostream>
#include <range/v3/view/enumerate.hpp>
#include <range/v3/view/reverse.hpp>
#include <range/v3/view/transform.hpp>
#include <range/v3/view/zip.hpp>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <unistd.h>
#include <utility>
#include <vector>

namespace ios	= boost::iostreams;
namespace lb	= libbio;
namespace mcs	= libbio::markov_chains;
namespace rsv	= ranges::views;


namespace {

	static std::string const sequence_characters{"ACGTUMRWSYKVHDBN-acgtumrwsykvhdbn"};
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
			header		= 0x1,
			sequence	= 0x2
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

		decltype(auto) to_tuple() const { return std::tie(type, content, extra); }
		bool operator==(fasta_line const &other) const { return to_tuple() == other.to_tuple(); }
	};


	std::ostream &operator<<(std::ostream &os, fasta_line const &line)
	{
		switch (line.type)
		{
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
			mcs::transition <initial_state, 		header_line,			1.0>,
			mcs::transition <header_line, 			sequence_line,			1.0>,
			mcs::transition <sequence_line, 		header_line,			0.5>,
			mcs::transition <sequence_line, 		sequence_line,			0.5>
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

	struct fasta_reader_delegate_ : public lb::fasta_reader_delegate
	{
		std::vector <fasta_line>	parsed_lines;
		std::string					current_sequence_line;

		bool handle_identifier(lb::fasta_reader_base &reader, std::string_view identifier, std::span <std::string_view const> extra_fields) override
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

		bool handle_sequence_chunk(lb::fasta_reader_base &reader, std::string_view sv, bool has_newline) override
		{
			current_sequence_line += sv;
			if (has_newline)
			{
				parsed_lines.emplace_back(fasta_line{sequence_line{std::move(current_sequence_line)}});
				current_sequence_line.clear();
			}
			return true;
		}
	};


	struct fasta_reader_delegate final : public fasta_reader_delegate_
	{
		bool handle_sequence_end(lb::fasta_reader_base &reader) override
		{
			return true;
		}
	};


	struct fasta_reader_line_by_line_delegate final : public fasta_reader_delegate_
	{
		bool should_continue{};

		bool handle_identifier(lb::fasta_reader_base &reader, std::string_view identifier, std::span <std::string_view const> extra_fields) override
		{
			should_continue = true;
			return fasta_reader_delegate_::handle_identifier(reader, identifier, extra_fields);
		}

		bool handle_sequence_end(lb::fasta_reader_base &reader) override
		{
			return false;
		}
	};


	class fasta_reader final : public lb::fasta_reader_base
	{
	private:
		void report_unexpected_character(int const current_state) const override
		{
			throw std::runtime_error{"Unexpected character"};
		}

		void report_unexpected_eof(int const current_state) const override
		{
			throw std::runtime_error{"Unexpected EOF"};
		}
	};


	template <typename t_cb>
	void test_fasta_reader(fasta_input_with_sequence_headers const &input, t_cb &&cb)
	{
		// Report the line types in the current input.
		fasta_line::line_type_ input_line_types_mask{};
		for (auto const &line : input.lines)
			input_line_types_mask |= std::to_underlying(line.type);
		RC_TAG(input_line_types_mask);

		std::array const blocksizes{0, 16, 64, 128, 256, 512, 1024, 2048};
		for (auto const blocksize : blocksizes)
		{
			RC_LOG() << "blocksize: " << blocksize << '\n';

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

			cb(read_handle, blocksize);
		}
	}


	bool compare_by_line(std::vector <fasta_line> const &lhs, std::vector <fasta_line> const &rhs)
	{
		if (lhs.size() != rhs.size())
		{
			RC_LOG() << "Size mismatch. lhs = " << lhs.size() << ", rhs = " << rhs.size() << '\n';
			return false;
		}

		bool retval = true;
		for (auto const &[lineno, tup] : rsv::zip(lhs, rhs) | rsv::enumerate)
		{
			auto const &[ll, rr] = tup;
			if (ll != rr)
			{
				RC_LOG() << "Mismatch on line " << lineno << ".\n" << "lhs: " << ll << "\nrhs: " << rr << '\n';
				retval = false;
			}
		}

		return retval;
	}
}


TEST_CASE(
	"fasta_reader can parse arbitrary input",
	"[fasta_reader]"
)
{
	return lb::rc_check(
		"fasta_reader can parse arbitrary input",
		[](fasta_input_with_sequence_headers const &input){
			test_fasta_reader(input, [&](lb::reading_handle &read_handle, auto const blocksize){
				fasta_reader reader;
				fasta_reader_delegate delegate;
				if (0 == blocksize)
					reader.parse(read_handle, delegate);
				else
					reader.parse(read_handle, delegate, blocksize);

				RC_ASSERT(compare_by_line(input.lines, delegate.parsed_lines));
			});
			return true;
		}
	);
}


TEST_CASE(
	"fasta_reader can parse arbitrary input one block at a time",
	"[fasta_reader]"
)
{
	return lb::rc_check(
		"fasta_reader can parse arbitrary input one block at a time",
		[](fasta_input_with_sequence_headers const &input){
			test_fasta_reader(input, [&](lb::reading_handle &read_handle, auto const blocksize){
				fasta_reader reader;
				fasta_reader_line_by_line_delegate delegate;
				reader.prepare();

				do
				{
					delegate.should_continue = false;
					if (0 == blocksize)
						reader.parse_(read_handle, delegate);
					else
						reader.parse_(read_handle, delegate, blocksize);
				} while (delegate.should_continue);

				RC_ASSERT(compare_by_line(input.lines, delegate.parsed_lines));
			});
			return true;
		}
	);
}

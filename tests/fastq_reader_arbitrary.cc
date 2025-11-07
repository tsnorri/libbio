/*
 * Copyright (c) 2025 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <array>
#include <libbio/fastq_reader.hh>
#include <libbio/file_handle.hh>
#include <libbio/file_handling.hh>
#include <libbio/rapidcheck_test_driver.hh>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <tuple>
#include <unistd.h>
#include <vector>

namespace ios	= boost::iostreams;
namespace lb	= libbio;


namespace {

	static std::string const header_characters{"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_"}; // .: not included currently.
	static std::string const sequence_characters{"ACGTUMRWSYKVHDBNacgtumrwsykvhdbn"}; // not all valid characters included.
	static std::string const quality_characters{"!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"};


	struct fastq_block
	{
		// FIXME: Add line wrapping.
		std::string header;
		std::string sequence;
		std::string quality;

		constexpr bool empty() const { return sequence.empty(); }
		constexpr auto to_tuple() const { return std::tie(header, sequence, quality); }
		constexpr bool operator==(fastq_block const &other) const { return to_tuple() == other.to_tuple(); }
	};


	struct fastq_input
	{
		std::vector <fastq_block> blocks;

		constexpr bool operator==(fastq_input const &other) const { return blocks == other.blocks; }
	};


	std::ostream &operator<<(std::ostream &os, fastq_block const &block)
	{
		os << '@' << block.header << '\n';
		os << block.sequence << '\n';
		os << "+\n"; // FIXME: test with the header.
		os << block.quality << '\n';
		return os;
	}


	std::ostream &operator<<(std::ostream &os, fastq_input const &input)
	{
		for (auto const &block : input.blocks)
			os << block;
		return os;
	}
}


namespace rc {

	template <>
	struct Arbitrary <fastq_block>
	{
		static Gen <fastq_block> arbitrary()
		{
			return gen::nonEmpty(
				gen::withSize([](int size){
					return gen::construct <fastq_block>(
						gen::nonEmpty(gen::container <std::string>(gen::elementOf(header_characters))),
						gen::container <std::string>(size, gen::elementOf(sequence_characters)),
						gen::container <std::string>(size, gen::elementOf(quality_characters))
					);
				})
			);
		}
	};


	template <>
	struct Arbitrary <fastq_input>
	{
		static Gen <fastq_input> arbitrary()
		{
			return gen::construct <fastq_input>(
				gen::container <std::vector <fastq_block>>(gen::arbitrary <fastq_block>())
			);
		}
	};
}


namespace {

	struct fastq_reader_delegate_ : public lb::fastq_reader_delegate
	{
		fastq_input input;

		bool handle_identifier(lb::fastq_reader_base &reader, std::string_view const &sv) override
		{
			auto &block{input.blocks.emplace_back()};
			block.header = sv;
			return true;
		}

		bool handle_sequence_chunk(lb::fastq_reader_base &reader, std::string_view const &sv, bool has_newline) override
		{
			input.blocks.back().sequence.append(sv);
			return true;
		}

		bool handle_sequence_end(lb::fastq_reader_base &reader) override
		{
			return true;
		}

		bool handle_quality_chunk(lb::fastq_reader_base &reader, std::string_view const &sv, bool has_newline) override
		{
			input.blocks.back().quality.append(sv);
			return true;
		}
	};


	struct fastq_reader_delegate final : public fastq_reader_delegate_
	{
		bool handle_quality_end(lb::fastq_reader_base &reader) override
		{
			return true;
		}
	};


	struct fastq_reader_line_by_line_delegate final : public fastq_reader_delegate_
	{
		bool should_continue{};

		bool handle_identifier(lb::fastq_reader_base &reader, std::string_view const &sv) override
		{
			should_continue = true;
			return fastq_reader_delegate_::handle_identifier(reader, sv);
		}

		bool handle_quality_end(lb::fastq_reader_base &reader) override
		{
			return false;
		}
	};


	class fastq_reader final : public lb::fastq_reader_base
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

		void report_length_mismatch(int const current_state) const override
		{
			throw std::runtime_error{"Sequence length mismatch"};
		}
	};


	template <typename t_cb>
	void test_fastq_reader(fastq_input const &input, t_cb &&cb)
	{
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

			cb(read_handle, blocksize);
		}
	}
}


TEST_CASE(
	"fastq_reader can parse arbitrary input",
	"[fastq_reader]"
)
{
	return lb::rc_check(
		"fastq_reader works with arbitrary input",
		[](fastq_input const &input){
			test_fastq_reader(input, [&](lb::reading_handle &read_handle, auto const blocksize){
				fastq_reader reader;
				fastq_reader_delegate delegate;
				if (0 == blocksize)
					reader.parse(read_handle, delegate);
				else
					reader.parse(read_handle, delegate, blocksize);

				RC_ASSERT(input == delegate.input);
			});
			return true;
		}
	);
}


TEST_CASE(
	"fastq_reader can parse arbitrary input one block at a time",
	"[fastq_reader]"
)
{
	return lb::rc_check(
		"fastq_reader can parse arbitrary input one block at a time",
		[](fastq_input const &input){
			test_fastq_reader(input, [&](lb::reading_handle &read_handle, auto const blocksize){
				fastq_reader reader;
				fastq_reader_line_by_line_delegate delegate;
				reader.prepare();

				do
				{
					delegate.should_continue = false;
					if (0 == blocksize)
						reader.parse_(read_handle, delegate);
					else
						reader.parse_(read_handle, delegate, blocksize);
				} while (delegate.should_continue);

				RC_ASSERT(input == delegate.input);
			});
			return true;
		}
	);
}

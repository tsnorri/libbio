/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <algorithm>
#include <boost/iostreams/stream.hpp>
#include <catch2/catch.hpp>
#include <libbio/bit_rle_codec.hh>
#include <vector>


namespace ios	= boost::iostreams;
namespace lb	= libbio;


namespace {
	
	struct input_value
	{
		std::uint64_t flag : 1;
		std::uint64_t count : 63;
		
		input_value(bool flag_, std::uint64_t count_):
			flag(flag_),
			count(count_)
		{
		}
	};
	
	bool operator==(input_value const lhs, input_value const rhs) { return lhs.flag == rhs.flag && lhs.count == rhs.count; }
	
	std::ostream &operator<<(std::ostream &os, input_value const val)
	{
		os << "Flag: " << +val.flag << " count: " << val.count;
		return os;
	}
	
	
	template <typename t_word>
	class test_input
	{
	public:
		
		typedef std::vector <t_word>		buffer_type;
		typedef std::vector <input_value>	value_vector;
		
	protected:
		buffer_type			m_buffer;
		value_vector		m_values;
		
	public:
		test_input(std::initializer_list <input_value> &&values):
			m_values(std::move(values))
		{
		}
		
		test_input(
			std::initializer_list <t_word> &&buffer_contents,
			std::initializer_list <input_value> &&values
		):
			m_buffer(std::move(buffer_contents)),
			m_values(std::move(values))
		{
		}
		
		buffer_type &buffer() { return m_buffer; }
		value_vector &values() { return m_values; }
		
		buffer_type const &buffer() const { return m_buffer; }
		value_vector const &values() const { return m_values; }
	};
	
	
	class fixture_1
	{
	public:
		typedef	std::uint16_t				word_type;
		typedef test_input <word_type>		test_input_type;
		
	protected:
		test_input_type		m_input;
		
	public:
		fixture_1():
			m_input({{1, 0x38e}, {0, 0x1003}, {1, 0x148224f14891aa9}, {0, 0xd1426a3}, {1, 0x2aaaaaaaaaaaaaaa}})
		{
			{
				std::initializer_list <std::uint8_t> values{
					0x83, 0x8e,
					0x10, 0x3,
					0x9a, 0xa9, 0xa9, 0x12, 0x89, 0x3c, 0x8a, 0x41,
					0x26, 0xa3, 0x1a, 0x28,
					0xaa, 0xaa, 0xd5, 0x55, 0xaa, 0xaa, 0xd5, 0x55, 0x80, 0x2
				};
				auto &buffer(m_input.buffer());
				buffer.resize(values.size() / sizeof(word_type));
				auto *data(reinterpret_cast <std::uint8_t *>(buffer.data()));
				std::copy(values.begin(), values.end(), data);
			}
		}
		
		test_input_type const &input() const { return m_input; }
		std::size_t const size() const { return m_input.buffer().size(); }
		std::size_t const byte_size() const { return sizeof(word_type) * size(); }
	};
}


SCENARIO("Bit runs can be read from a stream")
{
	GIVEN("A stream of words")
	{
		fixture_1 fx;
		
		WHEN("the items are read")
		{
			std::vector <input_value> values;
			ios::stream <ios::array_source> istream;
			auto const &buffer(fx.input().buffer());
			istream.open(reinterpret_cast <char const *>(buffer.data()), fx.byte_size());
			
			lb::bit_rle_decoder <fixture_1::word_type> decoder(istream);
			std::uint64_t count{};
			bool flag{};
			decoder.prepare();
			while (decoder.read_next_run(flag, count))
				values.emplace_back(flag, count);
			
			THEN("the input matches the expected")
			{
				auto const &expected(fx.input().values());
				REQUIRE(std::equal(expected.begin(), expected.end(), values.begin(), values.end()));
			}
		}
	}
}


SCENARIO("Bit runs can be written to a stream")
{
	GIVEN("A range of values")
	{
		fixture_1 fx;
		
		WHEN("the items are written")
		{
			std::vector <fixture_1::word_type> buffer(fx.size());
			ios::stream <ios::array_sink> ostream;
			ostream.open(reinterpret_cast <char *>(buffer.data()), fx.byte_size());
			
			lb::bit_rle_encoder <fixture_1::word_type> encoder(ostream);
			for (auto const &value : fx.input().values())
				encoder.write_run(value.flag, value.count);
			
			THEN("the output matches the expected")
			{
				auto const &expected(fx.input().buffer());
				REQUIRE(std::equal(expected.begin(), expected.end(), buffer.begin(), buffer.end()));
			}
		}
	}
}

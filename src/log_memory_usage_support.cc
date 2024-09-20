/*
 * Copyright (c) 2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <boost/endian/conversion.hpp> // boost::endian::big_to_native
#include <cstdio>
#include <libbio/assert.hh>
#include <libbio/log_memory_usage.hh>

namespace endian	= boost::endian;
namespace lb		= libbio;
namespace ml		= libbio::memory_logger;


namespace {
	
	enum header : std::uint32_t
	{
		states = 0x1
	};
	
	
	typedef ml::header_writer::buffer_type	buffer_type;
	
	
	template <typename t_type>
	void read_value(FILE *fp, t_type &value)
	{
		if (1 != ::fread(&value, sizeof(t_type), 1, fp))
			throw std::runtime_error(::strerror(errno));

		value = endian::big_to_native(value);
	}
	
	
	template <typename t_type>
	t_type read_value(FILE *fp)
	{
		t_type retval{};
		read_value(fp, retval);
		return retval;
	}
	
	
	template <typename t_type>
	void write_value_at(t_type value, char *dst)
	{
		value = endian::native_to_big(value);
		memcpy(dst, &value, sizeof(t_type));
	}
	
	
	template <typename t_type>
	void write_value(t_type value, buffer_type &buffer)
	{
		auto const pos(buffer.size());
		buffer.resize(pos + sizeof(t_type));
		write_value_at(value, buffer.data() + pos);
	}
	
	
	void write_string(char const * const str, buffer_type &buffer)
	{
		auto const pos(buffer.size());
		auto const len(std::strlen(str));
		buffer.resize(pos + 1 + len);
		memcpy(buffer.data() + pos, str, 1 + len); // Include the NUL character
	}
}


namespace libbio::memory_logger {
	
	// Header format:
	// ┌──────────────────────────────────────┬───────────────┐
	// │ version (0x1)                        │ std::uint32_t │
	// ├──────────────────────────────────────┼───────────────┤
	// │ total header length (starting below) │ std::uint32_t │
	// ├──────────────────────────────────────┴───────────────┤
	// │ Headers                                              │
	// │┌─────────────────────────────────────┬───────────────┤
	// ││ header                              │ std::uint32_t │
	// │├─────────────────────────────────────┴───────────────┤
	// ││ header-specific data                                │
	// └┴─────────────────────────────────────────────────────┘
	//
	// ┌──────────────────────────────────────┬───────────────┐
	// │ states (0x1)                         │ std::uint32_t │
	// ├──────────────────────────────────────┼───────────────┤
	// │ total length of the state list       │ std::uint32_t │
	// ├──────────────────────────────────────┴───────────────┤
	// │ State list                                           │
	// │┌─────────────────────────────────────┬───────────────┤
	// ││ state number                        │ std::uint64_t │
	// │├─────────────────────────────────────┼───────────────┤
	// ││ state name                          │ char*         │
	// └┴─────────────────────────────────────┴───────────────┘
	
	void header_writer::write_header(int const fd, header_writer_delegate &delegate)
	{
		m_buffer.clear();
		m_buffer.reserve(512);
		
		write_value(std::uint32_t(1), m_buffer); // Version
		auto const header_size_offset(m_buffer.size());
		write_value(std::uint32_t(0), m_buffer); // Size placeholder
		auto const header_list_offset(m_buffer.size());
		
		// States
		{
			write_value(std::uint32_t(header::states), m_buffer);
			auto const state_size_offset(m_buffer.size());
			write_value(std::uint32_t(0), m_buffer); // Size placeholder
			auto const state_list_offset(m_buffer.size());
			
			delegate.add_states(*this);
			write_value_at <std::uint32_t>(m_buffer.size() - state_list_offset, m_buffer.data() + state_size_offset);
		}
		
		// Total header size.
		write_value_at <std::uint32_t>(m_buffer.size() - header_list_offset, m_buffer.data() + header_size_offset);
		
		// Write to the file descriptor.
		{
			auto const size(::write(fd, m_buffer.data(), m_buffer.size()));
			if (-1 == size || std::size_t(size) != m_buffer.size())
				throw std::runtime_error(::strerror(errno));
		}
	}
	
	
	void header_reader::read_states(FILE *fp, std::uint32_t &header_length, header_reader_delegate &delegate)
	{
		if (header_length < sizeof(std::uint32_t))
			throw std::runtime_error("Unexpected end of file");
		
		// Read the state list length, update remaining header length.
		auto state_list_length(read_value <std::uint32_t>(fp));
		header_length -= sizeof(std::uint32_t);
		libbio_assert_lte(state_list_length, header_length);
		header_length -= state_list_length;
		
		// Allocate space for the state data.
		buffer_type buffer(state_list_length, 0);
		if (state_list_length != ::fread(buffer.data(), 1, state_list_length, fp))
			throw std::runtime_error(::strerror(errno));
		
		// Read the state index and label pairs.
		auto const *state_data(buffer.data());
		auto const * const state_data_end(state_data + state_list_length);
		while (state_data < state_data_end)
		{
			if ((state_data_end - state_data) < sizeof(std::uint64_t) + 1) // At least one NUL character in the label
				throw std::runtime_error("Unexpected end of file");
			
			// State index
			std::uint64_t state_index{};
			memcpy(&state_index, state_data, sizeof(std::uint64_t));
			state_index = endian::big_to_native(state_index);
			state_data += sizeof(std::uint64_t);
			
			// Label
			auto const * const label_end(static_cast <char const *>(memchr(state_data, '\0', state_data_end - state_data)));
			if (!label_end)
				throw std::runtime_error("Unexpected end of file");
			
			delegate.handle_state(*this, state_index, std::string_view{state_data, label_end});
				
			state_data = label_end + 1;
		}
	}
	
	
	void header_reader::read_header(FILE *fp, header_reader_delegate &delegate)
	{
		{
			auto const version(read_value <std::uint32_t>(fp));
			if (1 != version)
				throw std::runtime_error("Unexpected version");
		}
		
		auto header_length(read_value <std::uint32_t>(fp));
		while (header_length)
		{
			if (header_length < sizeof(std::uint32_t))
				throw std::runtime_error("Unexpected end of file");
			
			auto const header(read_value <std::uint32_t>(fp));
			header_length -= sizeof(std::uint32_t);
			
			switch (header)
			{
				case header::states:
					read_states(fp, header_length, delegate);
					break;
				
				default:
					throw std::runtime_error("Unexpected header");
			}
		}
	}
	
	
	void header_writer::add_state(char const *name, std::uint64_t const cast_value)
	{
		write_value(cast_value, m_buffer);
		write_string(name, m_buffer);
	}
	
	
	std::ostream &operator<<(std::ostream &os, event::type const type)
	{
		switch (type)
		{
			case event::type::allocated_amount:
				os << "allocated_amount";
				break;
				
			case event::type::marker:
				os << "marker";
				break;
			
			case event::type::unknown:
			default:
				os << "unknown";
				break;
		}
		
		return os;
	}
	
	
	std::ostream &operator<<(std::ostream &os, event const &evt)
	{
		switch (evt.event_type())
		{
			case event::type::allocated_amount:
				os << "m:" << evt.event_data();
				break;
				
			case event::type::marker:
				os << "M:" << evt.event_data();
				break;
			
			case event::type::unknown:
			default:
				os << "unknown";
				break;
		}
		
		return os;
	}
	
	
	void event::output_record(std::ostream &os) const
	{
		os << event_type();
		os << '\t';
		os << event_data();
	}
}

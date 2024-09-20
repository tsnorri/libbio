/*
 * Copyright (c) 2023-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_LOG_MEMORY_USAGE_HH
#define LIBBIO_LOG_MEMORY_USAGE_HH

#include <atomic>
#include <libbio/malloc_allocator.hh>
#include <ostream>
#include <type_traits>					// std::underlying_type_t
#include <utility>						// std::to_underlying


namespace libbio::memory_logger::detail {
	
#if defined(LIBBIO_LOG_ALLOCATED_MEMORY) && LIBBIO_LOG_ALLOCATED_MEMORY
	extern std::atomic_uint64_t s_current_state;
	extern std::atomic_uint64_t s_state_counter;
#endif
}


namespace libbio::memory_logger {
	
	struct header_writer_delegate;
	struct header_reader_delegate;
	
	
	class header_writer
	{
	public:
		typedef std::vector <char, malloc_allocator <char>> buffer_type;
		
	private:
		buffer_type	m_buffer;
		
	public:
		void write_header(int fd, header_writer_delegate &delegate);
		void add_state(char const *name, std::uint64_t const cast_value);
	};
	
	
	struct header_writer_delegate
	{
		virtual ~header_writer_delegate() {}
		virtual void add_states(header_writer &writer) = 0;
	};
	
	
	class header_reader
	{
	public:
		void read_header(FILE *fp, header_reader_delegate &delegate);
		
	private:
		void read_states(FILE *fp, std::uint32_t &header_length, header_reader_delegate &delegate);
	};
	
	
	struct header_reader_delegate
	{
		virtual ~header_reader_delegate() {}
		
		virtual void handle_state(
			header_reader &reader,
			std::uint64_t const idx,
			std::string_view const name
		) = 0;
	};
	
	
	template <typename t_state>
	struct header_writer_delegate_ : public header_writer_delegate
	{
		void add_states(header_writer &writer) final;
	};
	
	
	struct event
	{
		typedef std::uint64_t	value_type;
		
		enum class type : std::uint8_t
		{
			unknown				= 0,
			allocated_amount	= 1,
			marker				= 2
		};
		
		value_type data{};
		
		event() = default;
		
		explicit event(value_type data_):
			data(data_)
		{
		}
		
		event(value_type data_, type type_):
			data(data_)
		{
			data |= (value_type(std::to_underlying(type_)) << 56);
		}
		
		static inline event allocated_amount_event(value_type amt) { return event(amt, type::allocated_amount); }
		static inline event marker_event(value_type id) { return event(id, type::marker); }
		
		constexpr bool is_last_in_series() const { return bool(data >> 63); }
		constexpr type event_type() const { return static_cast <type>((data >> 56) & 0x7f); }
		constexpr value_type event_data() const { return data & 0xFFFFFFFFFFFFFFUL; } // We assume that long has 64 bits.
		
		void mark_last_in_series() { data |= (value_type(1) << 63); }
		
		constexpr operator value_type() const { return data; }
		
		void output_record(std::ostream &os) const;
		
		template <typename t_visitor>
		void visit(t_visitor &&visitor) const;
	};
	
	
	std::ostream &operator<<(std::ostream &os, event::type const type);
	std::ostream &operator<<(std::ostream &os, event const &evt);
	
	
	template <typename t_state>
	void header_writer_delegate_ <t_state>::add_states(header_writer &writer)
	{
		typedef std::underlying_type_t <t_state> state_type;
		for (state_type i(0); i < std::to_underlying(t_state::state_limit); ++i)
			writer.add_state(to_chars(static_cast <t_state>(i)), i);
	}
	
	
	template <typename t_visitor>
	void event::visit(t_visitor &&visitor) const
	{
		switch (event_type())
		{
			case type::unknown:				visitor.visit_unknown_event(*this); return;
			case type::allocated_amount:	visitor.visit_allocated_amount_event(*this); return;
			case type::marker:				visitor.visit_marker_event(*this); return;
		}
	}
	
	
	// State is a user-defined integer.
	// Adds a marker to the log if the state changes.
	inline std::uint64_t swap_current_state(std::uint64_t state)
	{
#if defined(LIBBIO_LOG_ALLOCATED_MEMORY) && LIBBIO_LOG_ALLOCATED_MEMORY
		auto const retval(detail::s_current_state.exchange(state, std::memory_order_relaxed));
		detail::s_state_counter.fetch_add(state, std::memory_order_release);
		return retval;
#else
		return 0;
#endif
	}
	
	
	template <typename t_type>
	requires std::is_scoped_enum_v <t_type>
	inline t_type swap_current_state(t_type state)
	{
#if defined(LIBBIO_LOG_ALLOCATED_MEMORY) && LIBBIO_LOG_ALLOCATED_MEMORY
		return std::bit_cast <t_type>(swap_current_state(std::bit_cast <std::uint64_t>(state)));
#else
		return state; // It is difficult to get a more meaningful return value.
#endif
	}
	
	
#if defined(LIBBIO_LOG_ALLOCATED_MEMORY) && LIBBIO_LOG_ALLOCATED_MEMORY
	template <typename t_type>
	requires (std::is_integral_v <t_type> || std::is_scoped_enum_v <t_type>)
	struct state_guard
	{
		t_type prev_state{};
		
		state_guard() = default;
		
		state_guard(t_type const state):
			prev_state(swap_current_state(state))
		{
		}
		
		~state_guard() { swap_current_state(prev_state); }
	};
#else
	template <typename t_type>
	requires (std::is_integral_v <t_type> || std::is_scoped_enum_v <t_type>)
	struct state_guard
	{
		state_guard(t_type const) {}
	};
#endif
}


namespace libbio {
	
	void setup_allocated_memory_logging_(memory_logger::header_writer_delegate &delegate);
	
	
	inline void setup_allocated_memory_logging(memory_logger::header_writer_delegate &delegate)
	{
#if defined(LIBBIO_LOG_ALLOCATED_MEMORY) && LIBBIO_LOG_ALLOCATED_MEMORY
		setup_allocated_memory_logging_(delegate);
#endif
	}
}

#endif

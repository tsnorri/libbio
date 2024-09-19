/*
 * Copyright (c) 2023-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_LOG_MEMORY_USAGE_HH
#define LIBBIO_LOG_MEMORY_USAGE_HH

#include <atomic>
#include <ostream>
#include <utility>	// std::to_underlying


namespace libbio::memory_logger::detail {
	
#if defined(LIBBIO_LOG_ALLOCATED_MEMORY) && LIBBIO_LOG_ALLOCATED_MEMORY
	extern std::atomic_uint64_t s_current_state;
#endif
}


namespace libbio::memory_logger {
	
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
	};
	
	
	std::ostream &operator<<(std::ostream &os, event::type const type);
	std::ostream &operator<<(std::ostream &os, event const &evt);
	
	
	// State is a user-defined integer.
	// Adds a marker to the log if the state changes.
	inline void set_current_state(std::uint64_t state)
	{
#if defined(LIBBIO_LOG_ALLOCATED_MEMORY) && LIBBIO_LOG_ALLOCATED_MEMORY
		detail::s_current_state.store(state, std::memory_order_relaxed);
#endif
	}
}


namespace libbio {
	
	void setup_allocated_memory_logging_();
	
	
	inline void setup_allocated_memory_logging()
	{
#if defined(LIBBIO_LOG_ALLOCATED_MEMORY) && LIBBIO_LOG_ALLOCATED_MEMORY
		setup_allocated_memory_logging_();
#endif
	}
}

#endif

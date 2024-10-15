/*
 * Copyright (c) 2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_BOUNDED_MPMC_QUEUE_HH
#define LIBBIO_BOUNDED_MPMC_QUEUE_HH

#include <atomic>
#include <libbio/assert.hh>
#include <libbio/bits.hh>	// libbio::bits::gte_power_of_2
#include <numeric>			// std::iota
#include <ranges>
#include <semaphore>
#include <vector>


namespace libbio {
	
	// Based on the CB-Queue in Orozco, D., Garcia, E., Khan, R., Livingston, K., & Gao, G. R. (2012).
	// Toward high-throughput algorithms on many-core architectures. ACM Transactions on Architecture
	// and Code Optimization (TACO), 8(4), 1-21.
	template <typename t_value>
	class bounded_mpmc_queue
	{
	public:
		typedef t_value			value_type;
		typedef std::uint16_t	size_type;
		typedef std::uint64_t	ticket_type;
		
		constexpr static size_type const MAX_SIZE{std::numeric_limits <size_type>::max()};
		
		struct start_from_reading
		{
			bool value{};
			constexpr operator bool() const { return value; }
		};
		
	protected:
		struct stored_index_
		{
			ticket_type	turn{};
			size_type	value{};
		};
		
		struct alignas(std::hardware_destructive_interference_size) stored_index
		{
			std::atomic <ticket_type>	turn{};
			std::atomic <size_type>		value{};
			
			/* implicit */ stored_index(stored_index_ const si):
				turn(si.turn),
				value(si.value)
			{
			}
			
			// FIXME: Remove when libstdc++ has support for std::vector’s std::from_range_t constructor.
			/* implicit */ stored_index(stored_index const &other):
				turn(other.turn.load(std::memory_order_relaxed)),
				value(other.value.load(std::memory_order_relaxed))
			{
			}
		};
		
		typedef std::vector <value_type>					value_vector;
		typedef std::vector <stored_index>					index_vector;
		typedef std::counting_semaphore <MAX_SIZE>			semaphore_type;
		
	protected:
		alignas(std::hardware_destructive_interference_size) std::atomic <ticket_type>	m_reader_ticket{};
		alignas(std::hardware_destructive_interference_size) std::atomic <ticket_type>	m_writer_ticket{};
		index_vector																	m_indices;
		value_vector																	m_values;
		size_type																		m_index_mask{};
		size_type																		m_index_bits{};
		
	private:
		static inline std::size_t queue_size(size_type const size_);
		static inline void wait_for_turn(std::atomic <ticket_type> &turn, ticket_type expected);

	public:
#if 0
		bounded_mpmc_queue(size_type const size_, std::size_t const queue_size_, bool is_readers_turn):
			m_writer_ticket(is_readers_turn ? queue_size_ : 0),
			m_indices(
				std::from_range_t{},
				std::views::iota(size_type(0), queue_size_)
				| std::views::transform([is_readers_turn](size_type const value){ return stored_index_{is_readers_turn, value}; })
			),
			m_values(queue_size_),
			m_index_mask(queue_size_ - 1),
			m_index_bits(libbio::bits::highest_bit_set(m_index_mask))
		{
			libbio_assert_lt(0, size());
		}
#endif

		// FIXME: Replace with the above when libstdc++ has support for std::vector’s std::from_range_t constructor.
		bounded_mpmc_queue(size_type const size_, std::size_t const queue_size_, bool is_readers_turn):
			m_writer_ticket(is_readers_turn ? queue_size_ : 0),
			m_values(queue_size_),
			m_index_mask(queue_size_ - 1),
			m_index_bits(libbio::bits::highest_bit_set(m_index_mask))
		{
			libbio_assert_lt(0, size());
			
			m_indices.reserve(queue_size_);
			for (std::size_t i(0); i < queue_size_; ++i)
				m_indices.emplace_back(stored_index_{is_readers_turn, size_type(i)});
		}
		
		explicit bounded_mpmc_queue(size_type const size_):
			bounded_mpmc_queue(size_, queue_size(size_), false)
		{
		}
		
		bounded_mpmc_queue(size_type const size_, start_from_reading const should_start_from_reading):
			bounded_mpmc_queue(size_, queue_size(size_), should_start_from_reading)
		{
		}

		constexpr size_type size() const { return m_values.size(); }
		
		value_vector &values() { return m_values; }
		value_type &operator[](size_type const idx) { libbio_assert_lt(idx, m_values.size()); return m_values[idx]; }
		
		size_type pop_index();
		value_type &pop() { return m_values[pop_index()]; }
		void push(value_type &value);
	};
	
	
	template <typename t_value>
	std::size_t bounded_mpmc_queue <t_value>::queue_size(size_type const size_)
	{
		auto const power(libbio::bits::gte_power_of_2(std::size_t(size_)));
		if (!power)
			throw std::range_error("Unable to construct a queue of the given size");
		
		if (MAX_SIZE < power - 1)
			throw std::range_error("Unable to construct a queue of the given size");
		
		return power;
	}
	
	
	template <typename t_value>
	void bounded_mpmc_queue <t_value>::wait_for_turn(std::atomic <ticket_type> &turn, ticket_type expected)
	{
		ticket_type prev(expected - ticket_type(1)); // Best guess so we can start waiting immediately.
		while (true)
		{
			turn.wait(prev, std::memory_order_acquire); // Blocks if equal.
			auto const curr(turn.load(std::memory_order_relaxed)); // Cannot be moved before wait() since it uses std::memory_order_acquire.
			if (curr == expected)
				return;
			prev = curr;
		}
	}
	
	
	template <typename t_value>
	auto bounded_mpmc_queue <t_value>::pop_index() -> size_type
	{
		auto const ticket(m_reader_ticket.fetch_add(1, std::memory_order_acquire));
		auto const turn(2 * (ticket >> m_index_bits) + 1);
		auto const pos(ticket & m_index_mask);
		auto &index(m_indices[pos]);
		wait_for_turn(index.turn, turn);
		auto const val_idx(index.value.load(std::memory_order_relaxed));
		index.turn.fetch_add(1, std::memory_order_release); 
		index.turn.notify_all();
		return val_idx;
	}
	
	
	template <typename t_value>
	void bounded_mpmc_queue <t_value>::push(value_type &val)
	{
		auto const val_idx(std::distance(&m_values.front(), &val));
		auto const ticket(m_writer_ticket.fetch_add(1, std::memory_order_acquire));
		auto const turn(2 * (ticket >> m_index_bits));
		auto const pos(ticket & m_index_mask);
		auto &index(m_indices[pos]);
		wait_for_turn(index.turn, turn);
		index.value.store(val_idx, std::memory_order_release);
		index.turn.fetch_add(1, std::memory_order_release);
		index.turn.notify_all();
	}
}

#endif

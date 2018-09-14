/*
 * Copyright (c) 2017-2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_COPYABLE_ATOMIC_HH
#define LIBBIO_COPYABLE_ATOMIC_HH

#include <atomic>


namespace libbio {
	
	template <typename t_value>
	class copyable_atomic final : public std::atomic <t_value>
	{
	public:
		using std::atomic <t_value>::atomic;
		
		copyable_atomic(copyable_atomic const &other)
		{
			this->store(other.load(std::memory_order_acquire), std::memory_order_release);
		}
		
		copyable_atomic &operator=(copyable_atomic const &other) &
		{
			this->store(other.load(std::memory_order_acquire), std::memory_order_release);
			return *this;
		}
	};
}

#endif

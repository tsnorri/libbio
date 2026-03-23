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
		typedef std::atomic <t_value> base_type;

		using base_type::atomic;

		using base_type::load;
		using base_type::store;

		using base_type::fetch_add;
		using base_type::fetch_sub;
		using base_type::fetch_and;
		using base_type::fetch_or;
		using base_type::fetch_xor;

#if defined(__cpp_lib_atomic_min_max) and 202403L <= __cpp_lib_atomic_min_max
		using base_type::fetch_min;
		using base_type::fetch_max;
#endif

		copyable_atomic():
			std::atomic <t_value>()
		{
		}

		copyable_atomic(copyable_atomic const &other)
		{
			this->store(other.load(std::memory_order_acquire), std::memory_order_release);
		}

		copyable_atomic &operator=(copyable_atomic const &other) &
		{
			if (this != &other)
				this->store(other.load(std::memory_order_acquire), std::memory_order_release);
			return *this;
		}
	};
}

#endif

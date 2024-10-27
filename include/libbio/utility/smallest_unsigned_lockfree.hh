/*
 * Copyright (c) 2018-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_UTILITY_SMALLEST_UNSIGNED_LOCKFREE_HH
#define LIBBIO_UTILITY_SMALLEST_UNSIGNED_LOCKFREE_HH

#include <atomic>
#include <cstdint>
#include <type_traits>


namespace libbio::detail {

	template <typename t_type, typename t_candidate, typename t_other>
	using smallest_unsigned_lockfree_type_gte_check_t = std::conditional_t <
		sizeof(t_type) <= sizeof(t_candidate) && std::atomic <t_candidate>::is_always_lock_free,
		t_candidate,
		t_other
	>;


	template <int t_count>
	struct smallest_unsigned_lockfree_type_gte_helper
	{
		template <typename t_type, typename t_first, typename... t_rest>
		using type = smallest_unsigned_lockfree_type_gte_check_t <
			t_type,
			t_first,
			typename smallest_unsigned_lockfree_type_gte_helper <sizeof...(t_rest)>::template type <t_type, t_rest...>
		>;
	};


	template <>
	struct smallest_unsigned_lockfree_type_gte_helper <1>
	{
		template <typename t_type, typename t_first>
		using type = smallest_unsigned_lockfree_type_gte_check_t <t_type, t_first, void>;
	};


	template <typename t_first, typename... t_types>
	using smallest_unsigned_lockfree_type_gte_helper_t =
	typename smallest_unsigned_lockfree_type_gte_helper <sizeof...(t_types)>::template type <t_first, t_types...>;
}


namespace libbio {

	template <typename t_type>
	struct smallest_unsigned_lockfree_type_gte
	{
		typedef typename detail::smallest_unsigned_lockfree_type_gte_helper_t <
			t_type, uint8_t, uint16_t, uint32_t, uint64_t
		> type;
	};

	template <typename t_type>
	using smallest_unsigned_lockfree_type_gte_t = typename smallest_unsigned_lockfree_type_gte <t_type>::type;
}

#endif

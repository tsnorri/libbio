/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_UTIL_HH
#define LIBBIO_UTIL_HH

#include <atomic>
#include <ostream>


namespace libbio { namespace detail {
	
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
}}


namespace libbio {
	
	// Make a type const conditionally.
	template <typename t_type, bool t_make_const>
	struct make_const
	{
		typedef t_type type;
	};
	
	template <typename t_type>
	struct make_const <t_type, true>
	{
		typedef t_type const type;
	};
	
	template <typename t_type, bool t_make_const>
	using make_const_t = typename make_const <t_type, t_make_const>::type;
	
	
	// Comparison helper.
	template <typename t_lhs, typename t_rhs>
	struct compare_lhs_first_lt
	{
		bool operator()(t_lhs const &lhs, t_rhs const &rhs) const
		{
			return lhs.first < rhs;
		}
	};
	
	
	template <typename t_type>
	struct smallest_unsigned_lockfree_type_gte
	{
		typedef typename detail::smallest_unsigned_lockfree_type_gte_helper_t <
			t_type, uint8_t, uint16_t, uint32_t, uint64_t
		> type;
	};
	
	template <typename t_type>
	using smallest_unsigned_lockfree_type_gte_t = typename smallest_unsigned_lockfree_type_gte <t_type>::type;
	
	
	template <typename t_src, typename t_dst>
	void resize_and_copy(t_src const &src, t_dst &dst)
	{
		dst.resize(src.size());
		std::copy(src.begin(), src.end(), dst.begin());
	}
	
	
	void log_time(std::ostream &stream);
	
	
	template <typename t_value, typename t_allocator>
	void clear_and_resize_vector(std::vector <t_value, t_allocator> &vec)
	{
		// Swap with an empty vector.
		std::vector <t_value, t_allocator> empty_vec;
		
		using std::swap;
		swap(vec, empty_vec);
	}
}

#endif

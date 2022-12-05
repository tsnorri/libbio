/*
 * Copyright (c) 2022 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_TUPLE_SLICE_HH
#define LIBBIO_TUPLE_SLICE_HH

#include <tuple>


namespace libbio::tuples {
	
	template <std::size_t t_lhs, std::size_t t_rhs, bool t_should_add_references = false, std::size_t t_i = t_lhs, typename t_tuple>
	auto slice(t_tuple &&tuple)
	{
		static_assert(
			!(t_should_add_references && std::is_rvalue_reference_v <t_tuple>),
			"Cannot return tuple elements by reference when the input is an rvalue reference."
		);
		
		if constexpr (t_i < t_lhs)
			return slice <t_lhs, t_rhs, t_should_add_references, 1 + t_i>(std::forward <t_tuple>(tuple));
		else if constexpr (t_rhs <= t_i)
			return std::tuple <>{};
		else
		{
			typedef std::tuple_element_t <t_i, std::remove_cvref_t <t_tuple>>	input_element_type;
			typedef std::conditional_t <
				t_should_add_references,
				std::add_lvalue_reference_t <input_element_type>,
				input_element_type
			>																	output_element_type;
			return std::tuple_cat(
				std::tuple <output_element_type>{std::get <t_i>(tuple)}, // std::get returns an rvalue if the input is one.
				slice <t_lhs, t_rhs, t_should_add_references, 1 + t_i>(std::forward <t_tuple>(tuple))
			);
		}
	}
	
	
	template <std::size_t t_lhs, bool t_should_add_references = false, typename t_tuple>
	auto slice(t_tuple &&tuple)
	{
		return slice <t_lhs, std::tuple_size_v <t_tuple>, t_should_add_references, t_lhs>(std::forward <t_tuple>(tuple));
	}
	
	
	template <typename t_tuple, std::size_t t_lhs, std::size_t t_rhs = std::tuple_size_v <t_tuple>, bool t_should_add_references = false>
	using slice_t = std::invoke_result_t <decltype(&slice <t_lhs, t_rhs, t_should_add_references, t_lhs, t_tuple>), t_tuple>;
}

#endif

/*
 * Copyright (c) 2022 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_TUPLE_ACCESS_HH
#define LIBBIO_TUPLE_ACCESS_HH

#include <tuple>


namespace libbio::tuples::detail {
	template <typename>
	struct visit_parameters {};
	
	
	template <typename... t_args>
	struct visit_parameters <std::tuple <t_args...>>
	{
		template <typename t_fn>
		consteval static decltype(auto) all(t_fn &&fn)
		{
			return fn.template operator() <t_args...>();
		}
		
		template <typename t_fn>
		constexpr static void each(t_fn &&fn)
		{
			(fn.template operator() <t_args>(), ...);
		}
	};
	
	
	template <std::size_t t_idx, typename t_tuple, typename t_missing, bool t_is_less_than>
	struct conditional_element_helper
	{
		typedef std::tuple_element_t <t_idx, t_tuple> type;
	};
	
	template <std::size_t t_idx, typename t_tuple, typename t_missing>
	struct conditional_element_helper <t_idx, t_tuple, t_missing, false>
	{
		typedef t_missing type;
	};
}


namespace libbio::tuples {
	
	// Nicer accessors and constants.
	
	typedef std::tuple <>	empty;
	
	template <typename t_tuple>
	using head_t = std::tuple_element_t <0, t_tuple>;
	
	
	template <typename t_tuple>
	using second_t = std::tuple_element_t <1, t_tuple>;
	
	
	template <std::size_t t_index>
	struct element_at_index
	{
		template <typename t_tuple>
		using type = std::tuple_element_t <t_index, t_tuple>;
	};
	
	
	template <typename t_tuple, typename t_default, std::size_t t_size = std::tuple_size_v <t_tuple>>
	struct head_
	{
		typedef head_t <t_tuple>	type;
	};
	
	template <typename t_tuple, typename t_default>
	struct head_ <t_tuple, t_default, 0>
	{
		typedef t_default	type;
	};
	
	template <typename t_tuple, typename t_default>
	using head_t_ = typename head_ <t_tuple, t_default>::type;
	
	
	template <typename t_tuple>
	using last = std::tuple_element <std::tuple_size_v <t_tuple> - 1, t_tuple>;
	
	template <typename t_tuple>
	using last_t = typename last <t_tuple>::type;
	
	
	template <std::size_t t_idx, typename t_tuple, typename t_missing = void>
	struct conditional_element
	{
		typedef detail::conditional_element_helper <t_idx, t_tuple, t_missing, t_idx < std::tuple_size_v <t_tuple>>::type type;
	};
	
	template <std::size_t t_idx, typename t_tuple, typename t_missing = void>
	using conditional_element_t = conditional_element <t_idx, t_tuple, t_missing>::type;
	
	
	template <typename, template <typename...> typename>
	struct forward {};
	
	template <typename... t_args, template <typename...> typename t_target>
	struct forward <std::tuple <t_args...>, t_target>
	{
		typedef t_target <t_args...> type;
	};
	
	template <typename t_type, template <typename...> typename t_target>
	using forward_t = forward <t_type, t_target>::type;
	
	
	template <template <typename...> typename t_tpl>
	struct forward_to
	{
		template <typename t_type>
		struct parameters_of {};
		
		template <typename... t_args>
		struct parameters_of <std::tuple <t_args...>>
		{
			typedef t_tpl <t_args...> type;
		};
		
		template <typename t_type>
		using parameters_of_t = parameters_of <t_type>::type;
	};
	
	
	// Access the parameters for use with e.g. a fold expression.
	template <typename t_tuple, typename t_fn>
	consteval inline decltype(auto) visit_all_parameters(t_fn &&fn)
	{
		return detail::visit_parameters <t_tuple>::all(fn);
	}
	
	
	template <typename t_tuple, typename t_fn>
	constexpr inline void visit_parameters(t_fn &&fn)
	{
		detail::visit_parameters <t_tuple>::each(fn);
	}
}

#endif

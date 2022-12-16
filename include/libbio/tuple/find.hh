/*
 * Copyright (c) 2022 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_TUPLE_FIND_HH
#define LIBBIO_TUPLE_FIND_HH

#include <libbio/tuple/access.hh>
#include <libbio/tuple/fold.hh>
#include <libbio/tuple/utility.hh>


namespace libbio::tuples {
	
	struct not_found {};
	
	template <typename t_tuple, template <typename> typename t_predicate, typename t_default = not_found>
	struct find_if
	{
		template <typename t_acc, typename t_type>
		using callback_type = std::conditional_t <
			t_predicate <t_type>::value,										// Check the predicate.
			std::tuple <append_t <head_t <t_acc>, t_type>, second_t <t_acc>>,	// Append to the first element if true.
			std::tuple <head_t <t_acc>, append_t <second_t <t_acc>, t_type>>	// Otherwise append to the second.
		>;
		
		typedef foldl_t <callback_type, std::tuple <empty, empty>, t_tuple>	type_;
		typedef head_t <type_>												matching_type;
		typedef second_t <type_>											rest_type;
		typedef head_t_ <matching_type, t_default>							first_matching_type;
		constexpr static inline bool const found{0 < std::tuple_size_v <matching_type>};
	};
	
	
	template <typename t_tuple, typename t_item, typename t_default = not_found>
	using find = find_if <t_tuple, same_as <t_item>::template with, t_default>;
}

#endif

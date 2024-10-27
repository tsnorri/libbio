/*
 * Copyright (c) 2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_BINARY_PARSING_FIELD_HH
#define LIBBIO_BINARY_PARSING_FIELD_HH

#include <libbio/binary_parsing/data_member.hh>
#include <libbio/binary_parsing/endian.hh>
#include <libbio/binary_parsing/range.hh>
#include <libbio/binary_parsing/read_value.hh>


namespace libbio::binary_parsing {

	template <data_member t_mem>
	struct field_
	{
		typedef decltype(t_mem)							data_member_type;
		typedef typename data_member_type::class_type	class_type;
		typedef typename data_member_type::type			type;

		type &get(class_type &obj) const { return obj.*(t_mem.value); }
		type const &get(class_type const &obj) const { return obj.*(t_mem.value); }

		type &operator()(class_type &obj) const { return get(obj); }
		type const &operator()(class_type const &obj) const { return get(obj); }

		// Customisation point.
		template <endian t_order>
		void read_value(range &rr, type &target) const { binary_parsing::read_value <t_order>(rr, target); }
	};


	template <data_member t_mem, template <data_member> typename t_base = field_>
	struct field : public t_base <t_mem>
	{
		typedef t_base <t_mem>					base_type;
		typedef typename base_type::class_type	class_type;

		template <endian t_order>
		void read_value(range &rr, class_type &obj) const { return base_type::template read_value <t_order>(rr, base_type::get(obj)); }
	};
}


namespace libbio::binary_parsing::detail {

	template <endian t_order, data_member t_mem, template <data_member> typename t_base, typename t_target>
	inline void read_field(range &rr, t_target &target)
	{
		binary_parsing::field <t_mem, t_base> field{};
		field.template read_value <t_order>(rr, target);
	}
}

#endif

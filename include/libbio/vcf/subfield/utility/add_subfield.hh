/*
 * Copyright (c) 2022 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_ADD_SUBFIELD_HH
#define LIBBIO_VCF_ADD_SUBFIELD_HH

#include <functional>
#include <libbio/vcf/subfield/base.hh>
#include <type_traits> // std::false_type, std::true_type


namespace libbio::vcf::detail {
	
	template <typename t_type>
	struct is_std_function : public std::false_type {};
	
	template <typename t_type>
	struct is_std_function <std::function <t_type>> : public std::true_type {};
	
	template <typename t_ret, typename ... t_args>
	struct is_std_function <std::function <t_ret(t_args...)>> : public std::true_type {};
	
	template <typename t_type>
	constexpr inline auto const is_std_function_v{is_std_function <t_type>::value};
	
	
	template <typename t_fn>
	constexpr inline bool is_callable_or_valid_std_function(t_fn &&fn)
	{
		// Currently we don’t make sure that fn is actually callable,
		// just that if it is a std::function, it is non-empty.
		if constexpr (is_std_function_v <t_fn>)
			return bool(fn);
		else
			return true;
	}
	
	
	constexpr inline bool compare_subfields(vcf::subfield_base const &lhs, vcf::subfield_base const &rhs)
	{
		return (
			lhs.metadata_value_type() == rhs.metadata_value_type() &&
			lhs.number() == rhs.number() &&
			lhs.value_type_is_vector() == rhs.value_type_is_vector()
		);
	}
}


namespace libbio::vcf {
	
	template <
		typename t_field_type,
		typename t_map,
		typename t_key,
		typename t_cb
		// typename t_field_base = typename t_map::mapped_type::element_type
	>
	void add_subfield(t_map &map, t_key const &key, t_cb &&cb)
	{
		// t_cb could be replaced with std::function <bool(std::string const &, t_field_base const &, t_field_base const &)>.
		
		auto const res(map.try_emplace(key));
		if (res.second)
		{
			// Did insert.
			res.first->second.reset(new t_field_type());
		}
		else
		{
			// Did not insert. Replace with a new value if needed.
			auto &ft_ptr(res.first->second);
			auto const &ft(*ft_ptr);
			t_field_type const ft_{};
			if (!detail::compare_subfields(ft, ft_) && detail::is_callable_or_valid_std_function(cb) && cb(key, ft, ft_))
				ft_ptr.reset(new t_field_type());
		}
	}
	
	
	template <typename t_field_type, typename t_map, typename t_key>
	void add_subfield(t_map &map, t_key const &key)
	{
		add_subfield <t_field_type>(map, key, [](std::string const &, auto const &, auto const &){ return true; });
	}
}

#endif

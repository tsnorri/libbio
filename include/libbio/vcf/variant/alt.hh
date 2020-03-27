/*
 * Copyright (c) 2020 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_VARIANT_ALT_HH
#define LIBBIO_VCF_VARIANT_ALT_HH

#include <libbio/cxxcompat.hh>
#include <libbio/types.hh>


namespace libbio {
	
	struct variant_alt_base
	{
		sv_type		alt_sv_type{};
	};
	
	
	template <typename t_string>
	struct variant_alt : public variant_alt_base
	{
		t_string	alt{};
		
		variant_alt() = default;
		variant_alt(variant_alt const &) = default;
		variant_alt(variant_alt &&) = default;
		
		variant_alt &operator=(variant_alt const &) & = default;
		variant_alt &operator=(variant_alt &&) & = default;

		// Allow copying from another specialization of variant_alt.
		template <typename t_other_string>
		explicit variant_alt(variant_alt <t_other_string> const &other):
			variant_alt_base(other),
			alt(other.alt)
		{
		}

		// Allow copying from another specialization of variant_alt.
		template <typename t_other_string>
		variant_alt &operator=(variant_alt <t_other_string> const &other) &
		{
			if (*this != other)
				alt = other.alt;
			return this;
		}
		
		void set_alt(std::string_view const &alt_) { alt = alt_; }
	};
	
	
	template <typename t_lhs_string, typename t_rhs_string>
	bool operator==(variant_alt <t_lhs_string> const &lhs, variant_alt <t_rhs_string> const &rhs)
	{
		return lhs.alt_sv_type == rhs.alt_sv_type && lhs.alt == rhs.alt;
	}

	template <typename t_lhs_string, typename t_rhs_string>
	bool operator<(variant_alt <t_lhs_string> const &lhs, variant_alt <t_rhs_string> const &rhs)
	{
		return to_underlying(lhs.alt_sv_type) < to_underlying(rhs.alt_sv_type) && lhs.alt < rhs.alt;
	}
}

#endif

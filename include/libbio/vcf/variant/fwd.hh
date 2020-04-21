/*
 * Copyright (c) 2017-2019 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VARIANT_FWD_HH
#define LIBBIO_VARIANT_FWD_HH

#include <libbio/cxxcompat.hh>
#include <string>
#include <type_traits>


namespace libbio::vcf::detail {
	class transient_variant_format_access;
	class variant_format_access;
}


namespace libbio::vcf {
	
	// Fwd.
	template <typename t_string, typename t_format_access>
	class formatted_variant;
	
	class variant;
	class transient_variant;
	
	class variant_sample_base;
	
	template <typename t_string>
	class variant_sample_tpl;
	
	typedef formatted_variant <std::string_view, detail::transient_variant_format_access>	transient_variant_formatted_base;
	typedef formatted_variant <std::string, detail::variant_format_access>					variant_formatted_base;

	typedef variant_sample_tpl <std::string_view>	transient_variant_sample;
	typedef variant_sample_tpl <std::string>		variant_sample;
	
	// Helper aliases.
	template <bool t_transient> using variant_formatted_base_t	= std::conditional_t <t_transient, transient_variant_formatted_base, variant_formatted_base>;
	template <bool t_transient> using variant_t					= std::conditional_t <t_transient, transient_variant, variant>;
	template <bool t_transient> using variant_sample_t			= std::conditional_t <t_transient, transient_variant_sample, variant_sample>;
}

#endif

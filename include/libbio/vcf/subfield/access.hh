/*
 * Copyright (c) 2020 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_SUBFIELD_ACCESS_HH
#define LIBBIO_VCF_SUBFIELD_ACCESS_HH

#include <libbio/vcf/subfield/value_access.hh>
#include <type_traits>


namespace libbio { namespace detail {
	
	// Templates for vcf_subfield_access’s base class. The necessary checks (e.g.
	// t_metadata_value_type == FLAG <=> t_number == 0) are handled in vcf_value_type_mapping and hence
	// not needed here.
	// vcf_vector_value_access_base does need the expected value count, though, so it is passed to the template
	// but not used for specialization here.
	
	// Template for checking a scalar type’s trivial destructibility.
	template <typename t_type, bool t_is_trivially_destructible = std::is_trivially_destructible_v <t_type>>
	struct vcf_scalar_subfield_base
	{
		typedef vcf_primitive_value_access <t_type> type;
	};
	
	template <typename t_type>
	struct vcf_scalar_subfield_base <t_type, false>
	{
		typedef vcf_object_value_access <t_type> type;
	};
	
	// Template for the given type.
	template <typename t_type, std::int32_t t_number>
	struct vcf_subfield_access_base
	{
		typedef typename vcf_scalar_subfield_base <t_type>::type type;
	};
	
	template <typename t_type, std::int32_t t_number>
	struct vcf_subfield_access_base <std::vector <t_type>, t_number>
	{
		typedef vcf_vector_value_access <t_type, t_number> type;
	};
	
	template <std::int32_t t_number, vcf_metadata_value_type t_metadata_value_type, bool t_is_transient>
	using vcf_subfield_access_base_t = typename vcf_subfield_access_base <
		vcf_value_type_mapping_t <
			t_metadata_value_type,
			vcf_value_count_corresponds_to_vector(t_number),
			t_is_transient
		>,
		t_number
	>::type;
}}


namespace libbio {
	
	// Handle a VCF field (specified in ##INFO or ##FORMAT).
	template <std::int32_t t_number, vcf_metadata_value_type t_metadata_value_type, bool t_is_transient>
	struct vcf_subfield_access : public detail::vcf_subfield_access_base_t <t_number, t_metadata_value_type, t_is_transient>
	{
	};
}

#endif

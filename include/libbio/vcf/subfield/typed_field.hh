/*
 * Copyright (c) 2020 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_SUBFIELD_TYPED_FIELD_HH
#define LIBBIO_VCF_SUBFIELD_TYPED_FIELD_HH

#include <libbio/vcf/subfield/decl.hh>
#include <libbio/vcf/subfield/genotype_field_base_decl.hh>
#include <libbio/vcf/subfield/info_field_base_decl.hh>
#include <libbio/vcf/subfield/type_mapping.hh>


namespace libbio { namespace detail {
	
	template <vcf_metadata_value_type t_value_type, bool t_is_vector, typename t_base, bool t_is_transient>
	struct vcf_typed_field_value_access
	{
		static_assert(std::is_same_v <t_base, vcf_info_field_base> || std::is_same_v <t_base, vcf_genotype_field_base>);
		
		typedef vcf_value_type_mapping_t <t_value_type, t_is_vector, t_is_transient>	value_type;
		typedef typename t_base::template container_tpl <t_is_transient>				container_type;
		
		virtual value_type &operator()(container_type &) const = 0;
		virtual value_type const &operator()(container_type const &) const = 0;
	};
}}


namespace libbio {
	
	// Virtual function declaration for operator().
	template <vcf_metadata_value_type t_value_type, bool t_is_vector, typename t_base>
	class vcf_typed_field :	public virtual t_base,
							public detail::vcf_typed_field_value_access <t_value_type, t_is_vector, t_base, false>,
							public detail::vcf_typed_field_value_access <t_value_type, t_is_vector, t_base, true>
	{
		static_assert(std::is_same_v <t_base, vcf_info_field_base> || std::is_same_v <t_base, vcf_genotype_field_base>);
		
	public:
		typedef t_base virtual_base;
		
	protected:
		template <bool B>
		using value_access_t = detail::vcf_typed_field_value_access <t_value_type, t_is_vector, t_base, B>;
		
	public:
		using value_access_t <false>::operator();
		using value_access_t <true>::operator();
		
		// For now these are only declared in the field types that use the mapping in the beginning of this header.
		// Since the GT field has a custom value type (as we parse the phasing information), it does not use the mapping.
		constexpr bool uses_vcf_type_mapping() const final { return true; }
		
		constexpr static bool is_typed_field() { return true; }
		
		constexpr bool value_type_is_vector() const							{ return t_is_vector; }
		constexpr vcf_metadata_value_type value_type() const override final	{ return t_value_type; }
	};
}

#endif

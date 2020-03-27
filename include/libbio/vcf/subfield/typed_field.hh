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
	
	// Virtualize value_type_is_vector(), get_value_type().
	template <typename t_base>
	struct vcf_typed_field_base : public virtual t_base
	{
		// For now these are only declared in the field types that use the mapping in the beginning of this header.
		// Since the GT field has a custom value type (as we parse the phasing information), it does not use the mapping.
		bool uses_vcf_type_mapping() const final { return true; }
	};
	
	
	template <vcf_metadata_value_type t_value_type, bool t_is_vector, template <bool> typename t_container, bool t_is_transient>
	struct vcf_typed_field_value_access
	{
		typedef vcf_value_type_mapping_t <t_value_type, t_is_vector, t_is_transient>	value_type;
		typedef t_container <t_is_transient>											container_type;
		
		virtual value_type &operator()(container_type &) const = 0;
		virtual value_type const &operator()(container_type const &) const = 0;
	};
}}


namespace libbio {
	
	// Virtual function declaration for operator().
	template <vcf_metadata_value_type t_value_type, bool t_is_vector, template <bool> typename t_container, typename t_base>
	class vcf_typed_field :	public detail::vcf_typed_field_base <t_base>,
								public detail::vcf_typed_field_value_access <t_value_type, t_is_vector, t_container, false>,
								public detail::vcf_typed_field_value_access <t_value_type, t_is_vector, t_container, true>
	{
		static_assert(std::is_same_v <t_base, vcf_info_field_base> || std::is_same_v <t_base, vcf_genotype_field_base>);
		
	protected:
		template <bool B> using value_access_t = detail::vcf_typed_field_value_access <t_value_type, t_is_vector, t_container, B>;
		
	public:
		using value_access_t <true>::operator();
		using value_access_t <false>::operator();
		
		constexpr static bool is_typed_field() { return true; }
		
		constexpr bool value_type_is_vector() const							{ return t_is_vector; }
		constexpr vcf_metadata_value_type value_type() const override final	{ return t_value_type; }
	};
}

#endif
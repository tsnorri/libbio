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


namespace libbio::vcf::detail {
	
	template <metadata_value_type t_value_type, bool t_is_vector, typename t_base, bool t_is_transient>
	struct typed_field_value_access
	{
		typedef value_type_mapping_t <t_value_type, t_is_vector, t_is_transient>	value_type;
		typedef typename t_base::template container_tpl <t_is_transient>			container_type;
		
		static_assert(std::is_same_v <t_base, info_field_base> || std::is_same_v <t_base, genotype_field_base>);
		static_assert(std::is_same_v <container_type, std::remove_cvref_t <container_type>>);
		
		virtual value_type &operator()(container_type &) const = 0;
		virtual value_type const &operator()(container_type const &) const = 0;
	};
	
	
	template <typename, bool>
	class generic_field_ds_access;
}


namespace libbio::vcf {
	
	// Virtual function declaration for operator().
	template <metadata_value_type t_value_type, bool t_is_vector, typename t_base>
	class typed_field :	public virtual t_base,
						public virtual detail::typed_field_value_access <t_value_type, t_is_vector, t_base, false>,
						public virtual detail::typed_field_value_access <t_value_type, t_is_vector, t_base, true>
	{
		static_assert(std::is_same_v <t_base, info_field_base> || std::is_same_v <t_base, genotype_field_base>);
		
		template <typename, bool>
		friend class detail::generic_field_ds_access;
		
	public:
		typedef t_base virtual_base;
		
	protected:
		template <bool B>
		using value_access_t = detail::typed_field_value_access <t_value_type, t_is_vector, t_base, B>;
		
	public:
		template <bool B>
		using value_type_tpl = typename value_access_t <B>::value_type;
		
		template <bool B>
		using container_type_tpl = typename value_access_t <B>::container_type;
		
	public:
		// For now these are only declared in the field types that use the mapping in the beginning of this header.
		// Since the GT field has a custom value type (as we parse the phasing information), it does not use the mapping.
		constexpr bool uses_vcf_type_mapping() const final { return true; }
		
		constexpr static bool is_typed_field() { return true; }
		
		constexpr bool value_type_is_vector() const						{ return t_is_vector; }
		constexpr metadata_value_type value_type() const override final	{ return t_value_type; }
	};
}

#endif

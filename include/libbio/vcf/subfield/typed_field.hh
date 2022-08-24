/*
 * Copyright (c) 2020 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_SUBFIELD_TYPED_FIELD_HH
#define LIBBIO_VCF_SUBFIELD_TYPED_FIELD_HH

#include <libbio/vcf/subfield/decl.hh>
#include <libbio/vcf/subfield/genotype_field_base_decl.hh>
#include <libbio/vcf/subfield/info_field_base_decl.hh>
#include <libbio/vcf/subfield/utility/type_mapping.hh>


namespace libbio::vcf {
	
	// Virtual function declaration for operator().
	template <metadata_value_type t_value_type, bool t_is_vector, typename t_base>
	class typed_field :	public t_base
	{
		static_assert(std::is_same_v <t_base, info_field_base> || std::is_same_v <t_base, genotype_field_base>);
		
	public:
		typedef value_type_mapping_t <t_value_type, t_is_vector, false>	value_type;
		typedef value_type_mapping_t <t_value_type, t_is_vector, true>	transient_value_type;
		typedef typename t_base::container_type							container_type;
		typedef typename t_base::transient_container_type				transient_container_type;
		
	public:
		// For now these are only declared in the field types that use the mapping in the beginning of this header.
		// Since the GT field has a custom value type (as we parse the phasing information), it does not use the mapping.
		constexpr bool uses_vcf_type_mapping() const final { return true; }
		
		constexpr static bool is_typed_field() { return true; }
		
		constexpr bool value_type_is_vector() const										{ return t_is_vector; }
		constexpr enum metadata_value_type metadata_value_type() const override final	{ return t_value_type; }
		
		virtual value_type &operator()(container_type &) const = 0;
		virtual value_type const &operator()(container_type const &) const = 0;
		virtual transient_value_type &operator()(transient_container_type &) const = 0;
		virtual transient_value_type const &operator()(transient_container_type const &) const = 0;
	};
}

#endif

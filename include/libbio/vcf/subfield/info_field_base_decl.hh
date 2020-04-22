/*
 * Copyright (c) 2019-2020 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_SUBFIELD_INFO_FIELD_BASE_DECL_HH
#define LIBBIO_VCF_SUBFIELD_INFO_FIELD_BASE_DECL_HH

#include <libbio/cxxcompat.hh>
#include <libbio/vcf/metadata.hh>
#include <libbio/vcf/subfield/base.hh>
#include <libbio/vcf/variant/abstract_variant_decl.hh>
#include <libbio/vcf/variant/fwd.hh>


namespace libbio::vcf {
	
	class abstract_variant;
	class metadata_info;
	
	
	// Base class for info field descriptions.
	class info_field_base :	public virtual subfield_base,
							public subfield_ds_access <variant_base_t, true>,
							public subfield_ds_access <variant_base_t, false>
	{
		friend class reader;
		
		template <typename, typename>
		friend class formatted_variant;
		
	public:
		template <bool t_is_transient> 
		using container_tpl = variant_base_t <t_is_transient>;
		
		template <bool t_is_vector, metadata_value_type t_value_type>
		using typed_field_type = typed_info_field_t <t_is_vector, t_value_type>;
		
	protected:
		metadata_info	*m_metadata{};
		
	protected:
		using subfield_ds_access <variant_base_t, true>::reset;
		using subfield_ds_access <variant_base_t, false>::reset;
		using subfield_ds_access <variant_base_t, true>::construct_ds;
		using subfield_ds_access <variant_base_t, false>::construct_ds;
		using subfield_ds_access <variant_base_t, true>::destruct_ds;
		using subfield_ds_access <variant_base_t, false>::destruct_ds;
		using subfield_ds_access <variant_base_t, true>::copy_ds;
		using subfield_ds_access <variant_base_t, false>::copy_ds;
		
		// Parse the contents of a string_view and assign the value to the variant.
		// Needs to be overridden.
		virtual bool parse_and_assign(std::string_view const &sv, transient_variant &var, std::byte *mem) const = 0;
		
		// Assign a value, used for FLAG.
		virtual bool assign(std::byte *mem) const { throw std::runtime_error("Not implemented"); };
		
		// For use with reader and variant classes:
		inline void prepare(transient_variant &dst) const;
		inline void parse_and_assign(std::string_view const &sv, transient_variant &dst) const;
		inline void assign_flag(transient_variant &dst) const;
		virtual info_field_base *clone() const override = 0;
		
		// Access the container’s buffer, for use with operator().
		std::byte *buffer_start(abstract_variant const &ct) const { return ct.m_info.get(); }
		
	public:
		using subfield_ds_access <variant_base_t, true>::output_vcf_value;
		using subfield_ds_access <variant_base_t, false>::output_vcf_value;
		
		metadata_info *get_metadata() const final { return m_metadata; }
		
		// Output the given separator and the field contents to a stream if the value in present in the variant.
		template <typename t_string, typename t_format_access>
		bool output_vcf_value(
			std::ostream &stream,
			formatted_variant <t_string, t_format_access> const &var,
			char const *sep
		) const;
		
		// Check whether the variant has a value for this INFO field.
		inline bool has_value(abstract_variant const &var) const;
	};
	
	
	typedef std::map <
		std::string,
		std::unique_ptr <info_field_base>,
		libbio::compare_strings_transparent
	>												info_field_map;
	
	typedef std::vector <info_field_base *>			info_field_ptr_vector;
	
	void add_reserved_info_keys(info_field_map &dst);
}

#endif

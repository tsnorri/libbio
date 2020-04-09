/*
 * Copyright (c) 2019-2020 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_SUBFIELD_BASE_HH
#define LIBBIO_VCF_SUBFIELD_BASE_HH

#include <libbio/types.hh>
#include <libbio/vcf/subfield/type_mapping.hh>
#include <libbio/vcf/variant/fwd.hh>
#include <ostream>


namespace libbio { namespace detail {
	
	// Fwd.
	template <typename>
	struct vcf_typed_field_base;
}}


namespace libbio {
	
	// Fwd.
	class vcf_metadata_base;
	class vcf_info_field_base;
	class vcf_genotype_field_base;
	
	typedef detail::vcf_typed_field_base <vcf_info_field_base>		vcf_typed_info_field_base;
	typedef detail::vcf_typed_field_base <vcf_genotype_field_base>	vcf_typed_genotype_field_base;
	
	template <vcf_metadata_value_type t_value_type, bool t_is_vector, template <bool> typename t_container, typename t_base>
	class vcf_typed_field;
	
	// Aliases for easier castability.
	template <bool t_is_vector, vcf_metadata_value_type t_value_type>
	using vcf_typed_info_field_t = vcf_typed_field <t_value_type, t_is_vector, variant_base_t, vcf_info_field_base>;

	template <bool t_is_vector, vcf_metadata_value_type t_value_type>
	using vcf_typed_genotype_field_t = vcf_typed_field <t_value_type, t_is_vector, variant_sample_t, vcf_genotype_field_base>;
	
	
	// Base class and interface for field descriptions (specified by ##INFO, ##FORMAT).
	class vcf_subfield_base
	{
		friend class vcf_metadata_format;
		friend class vcf_metadata_formatted_field;
		
	public:
		enum { INVALID_OFFSET = UINT16_MAX };
		
	protected:
		std::uint16_t		m_offset{INVALID_OFFSET};	// Offset of this field in the memory block.
		
	public:
		virtual ~vcf_subfield_base() {}

		// Value type according to the VCF header.
		virtual vcf_metadata_value_type value_type() const = 0;
		
		// Number of items in this field according to the VCF header.
		virtual std::int32_t number() const = 0;
		bool value_type_is_vector() const { return vcf_value_count_corresponds_to_vector(number()); }
		
		// Get the metadata pointer.
		virtual vcf_metadata_base *get_metadata() const = 0;

		// Whether the field uses the enumerated VCF type mapping.
		virtual bool uses_vcf_type_mapping() const { return false; }
		
	protected:
		std::uint16_t get_offset() const { return m_offset; }
		void set_offset(std::uint16_t offset) { m_offset = offset; }
		
		// Alignment of this field.
		virtual std::uint16_t alignment() const = 0;
		
		// Size of this field in bytes.
		virtual std::uint16_t byte_size() const = 0;
		
		// Create a clone of this object.
		virtual vcf_subfield_base *clone() const = 0;
	};
	
	
	// Interface for the container in question (one of variant, transient_variant, variant_sample, transient_variant_sample).
	template <template <bool> typename t_container_tpl, bool t_is_transient>
	class vcf_subfield_ds_access
	{
	public:
		virtual ~vcf_subfield_ds_access() {}
		
	protected:
		typedef t_container_tpl <t_is_transient>	container_type;
		typedef t_container_tpl <false>				non_transient_container_type;
		
		// Mark the target s.t. no values have been read yet. (Currently used for vectors.)
		virtual void reset(container_type const &ct, std::byte *mem) const = 0;
		
		// Construct the data structure.
		virtual void construct_ds(container_type const &ct, std::byte *mem, std::uint16_t const alt_count) const = 0;
		
		// Destruct the datastructure.
		virtual void destruct_ds(container_type const &ct, std::byte *mem) const = 0;
		
		// access_ds left out on purpose since its return type varies.
		
		// Copy the data structure.
		virtual void copy_ds(
			container_type const &src_ct,
			non_transient_container_type const &dst_ct,
			std::byte const *src,
			std::byte *dst
		) const = 0;
			
	public:
		// Output the field contents to a stream.
		// In case of vcf_info_field, the value has to be present in the variant.
		virtual void output_vcf_value(std::ostream &stream, container_type const &ct) const = 0;
	};
}

#endif

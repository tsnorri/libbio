/*
 * Copyright (c) 2019-2020 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_SUBFIELD_GENOTYPE_FIELD_BASE_DECL_HH
#define LIBBIO_VCF_SUBFIELD_GENOTYPE_FIELD_BASE_DECL_HH

#include <libbio/cxxcompat.hh>
#include <libbio/vcf/metadata.hh>
#include <libbio/vcf/subfield/base.hh>
#include <libbio/vcf/variant/fwd.hh>
#include <libbio/vcf/variant/sample.hh>


namespace libbio {
	
	// Base class for genotype field descriptions.
	class vcf_genotype_field_base :	public virtual vcf_subfield_base,
									public vcf_subfield_ds_access <variant_sample_t, true>,
									public vcf_subfield_ds_access <variant_sample_t, false>
	{
		friend class vcf_reader;
		
		template <typename, typename>
		friend class formatted_variant;
		
	public:
		enum { INVALID_INDEX = UINT16_MAX };
		
	public:
		template <bool t_is_transient>
		using container_tpl = variant_sample_t <t_is_transient>;
		
		template <bool t_is_vector, vcf_metadata_value_type t_value_type>
		using typed_field_type = vcf_typed_genotype_field_t <t_is_vector, t_value_type>;
		
	protected:
		vcf_metadata_format	*m_metadata{};
		std::uint16_t		m_index{INVALID_INDEX};	// Index of this field in the memory block.
		
	protected:
		using vcf_subfield_ds_access <variant_sample_t, true>::reset;
		using vcf_subfield_ds_access <variant_sample_t, false>::reset;
		using vcf_subfield_ds_access <variant_sample_t, true>::construct_ds;
		using vcf_subfield_ds_access <variant_sample_t, false>::construct_ds;
		using vcf_subfield_ds_access <variant_sample_t, true>::destruct_ds;
		using vcf_subfield_ds_access <variant_sample_t, false>::destruct_ds;
		using vcf_subfield_ds_access <variant_sample_t, true>::copy_ds;
		using vcf_subfield_ds_access <variant_sample_t, false>::copy_ds;
		
		// Parse the contents of a string_view and assign the value to the sample.
		// Needs to be overridden.
		virtual bool parse_and_assign(std::string_view const &sv, transient_variant_sample &sample, std::byte *mem) const = 0;
		
		inline void prepare(transient_variant_sample &dst) const;
		inline void parse_and_assign(std::string_view const &sv, transient_variant_sample &dst) const;
		virtual vcf_genotype_field_base *clone() const override = 0;
		
		// Access the container’s buffer, for use with operator().
		std::byte *buffer_start(variant_sample_base const &vs) const { return vs.m_sample_data.get(); }
		
	public:
		std::uint16_t get_index() const { return m_index; }
		void set_index(std::uint16_t index) { m_index = index; }
		
	public:
		using vcf_subfield_ds_access <variant_sample_t, true>::output_vcf_value;
		using vcf_subfield_ds_access <variant_sample_t, false>::output_vcf_value;
		
		vcf_metadata_format *get_metadata() const final { return m_metadata; }
		
		// Check whether the sample has a value for this genotype field.
		// (The field could just be removed from FORMAT but instead the
		// specification allows MISSING value to be specified. See VCF 4.3 Section 1.6.2.)
		inline bool has_value(variant_sample_base const &sample) const;
	};
	
	
	typedef std::map <
		std::string,
		std::unique_ptr <vcf_genotype_field_base>,
		libbio::compare_strings_transparent
	>													vcf_genotype_field_map;
	typedef std::vector <vcf_genotype_field_base *>		vcf_genotype_ptr_vector;
	
	void add_reserved_genotype_keys(vcf_genotype_field_map &dst);
}

#endif

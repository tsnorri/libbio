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


namespace libbio::vcf {
	
	class variant_format;
	
	
	// Base class for genotype field descriptions.
	class genotype_field_base :	public virtual subfield_base,
								public subfield_ds_access <variant_sample_t, true>,
								public subfield_ds_access <variant_sample_t, false>
	{
		friend class reader;
		
		template <typename, typename>
		friend class formatted_variant;
		
		friend bool operator==(variant_format const &, variant_format const &);
		
	public:
		enum { INVALID_INDEX = UINT16_MAX };
		
	public:
		template <bool t_is_transient>
		using container_tpl = variant_sample_t <t_is_transient>;
		
		template <bool t_is_vector, metadata_value_type t_value_type>
		using typed_field_type = typed_genotype_field_t <t_is_vector, t_value_type>;
		
	protected:
		metadata_format	*m_metadata{};
		std::uint16_t	m_index{INVALID_INDEX};	// Index of this field in the memory block.
		
	protected:
		using subfield_ds_access <variant_sample_t, true>::reset;
		using subfield_ds_access <variant_sample_t, false>::reset;
		using subfield_ds_access <variant_sample_t, true>::construct_ds;
		using subfield_ds_access <variant_sample_t, false>::construct_ds;
		using subfield_ds_access <variant_sample_t, true>::destruct_ds;
		using subfield_ds_access <variant_sample_t, false>::destruct_ds;
		using subfield_ds_access <variant_sample_t, true>::copy_ds;
		using subfield_ds_access <variant_sample_t, false>::copy_ds;
		
		// Parse the contents of a string_view and assign the value to the sample.
		// Needs to be overridden.
		virtual bool parse_and_assign(std::string_view const &sv, transient_variant_sample &sample, std::byte *mem) const = 0;
		
		inline void prepare(transient_variant_sample &dst) const;
		inline void parse_and_assign(std::string_view const &sv, transient_variant_sample &dst) const;
		virtual genotype_field_base *clone() const override = 0;
		
		// Access the container’s buffer, for use with operator().
		std::byte *buffer_start(variant_sample_base const &vs) const { return vs.m_sample_data.get(); }
		
	public:
		std::uint16_t get_index() const { return m_index; }
		void set_index(std::uint16_t index) { m_index = index; }
		
	public:
		using subfield_ds_access <variant_sample_t, true>::output_vcf_value;
		using subfield_ds_access <variant_sample_t, false>::output_vcf_value;
		
		metadata_format *get_metadata() const final { return m_metadata; }
		
		// Check whether the sample has a value for this genotype field.
		// (The field could just be removed from FORMAT but instead the
		// specification allows MISSING value to be specified. See VCF 4.3 Section 1.6.2.)
		inline bool has_value(variant_sample_base const &sample) const;
	};
	
	
	typedef std::map <
		std::string,
		std::unique_ptr <genotype_field_base>,
		libbio::compare_strings_transparent
	>												genotype_field_map;
	typedef std::vector <genotype_field_base *>		genotype_ptr_vector;
	
	void add_reserved_genotype_keys(genotype_field_map &dst);
}

#endif

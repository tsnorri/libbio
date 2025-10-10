/*
 * Copyright (c) 2019-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_SUBFIELD_GENOTYPE_FIELD_BASE_DECL_HH
#define LIBBIO_VCF_SUBFIELD_GENOTYPE_FIELD_BASE_DECL_HH

#include <cstddef>
#include <cstdint>
#include <libbio/utility/compare_strings_transparent.hh>
#include <libbio/vcf/constants.hh>
#include <libbio/vcf/metadata.hh>
#include <libbio/vcf/subfield/base.hh>
#include <libbio/vcf/variant/fwd.hh>
#include <libbio/vcf/variant/sample.hh>
#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>


namespace libbio::vcf::detail {
	class metadata_setup_helper; // Fwd.
}


namespace libbio::vcf {

	class variant_format;


	// Base class for genotype field descriptions.
	class genotype_field_base :	public subfield_base
	{
		friend class reader;
		friend class detail::metadata_setup_helper;

		template <typename, typename>
		friend class formatted_variant;

		friend bool operator==(variant_format const &, variant_format const &);

	public:
		constexpr static std::uint16_t INVALID_INDEX{UINT16_MAX};

	protected:
		template <bool B>
		using container_tpl = variant_sample_t <B>;

	public:
		template <enum metadata_value_type M, bool B>
		using typed_field_type = typed_genotype_field_t <M, B>;

		typedef variant_sample_t <false>	container_type;
		typedef variant_sample_t <true>		transient_container_type;

	protected:
		metadata_format	*m_metadata{};
		std::uint16_t	m_index{INVALID_INDEX};	// Index of this field in the memory block.

	protected:
		// Mark the target s.t. no values have been read yet. (Currently used for vectors.)
		virtual void reset(container_type const &ct, std::byte *mem) const = 0;
		virtual void reset(transient_container_type const &ct, std::byte *mem) const = 0;

		// Construct the data structure.
		virtual void construct_ds(container_type const &ct, std::byte *mem, std::uint16_t const alt_count) const = 0;
		virtual void construct_ds(transient_container_type const &ct, std::byte *mem, std::uint16_t const alt_count) const = 0;

		// Destruct the datastructure.
		virtual void destruct_ds(container_type const &ct, std::byte *mem) const = 0;
		virtual void destruct_ds(transient_container_type const &ct, std::byte *mem) const = 0;

		// Copy the data structure.
		virtual void copy_ds(
			transient_container_type const &src_ct,
			container_type const &dst_ct,
			std::byte const *src,
			std::byte *dst
		) const = 0;

		virtual void copy_ds(
			container_type const &src_ct,
			container_type const &dst_ct,
			std::byte const *src,
			std::byte *dst
		) const = 0;

		// Parse the contents of a string_view and assign the value to the sample.
		// Needs to be overridden.
		virtual bool parse_and_assign(std::string_view const &sv, transient_variant const &var, transient_variant_sample &sample, std::byte *mem) const = 0;

		inline void prepare(transient_variant_sample &dst) const;
		inline void parse_and_assign(std::string_view const &sv, transient_variant const &var, transient_variant_sample &dst) const;
		virtual genotype_field_base *clone() const override = 0;

		// Access the container’s buffer, for use with operator().
		std::byte *buffer_start(variant_sample_base const &vs) const { return vs.m_sample_data.get(); }

	public:
		constexpr std::uint16_t get_index() const { return m_index; }
		constexpr void set_index(std::uint16_t index) { m_index = index; }

	public:
		// Output the field contents to a stream.
		// In case of info_field, the value has to be present in the variant.
		virtual void output_vcf_value(std::ostream &stream, container_type const &ct) const = 0;
		virtual void output_vcf_value(std::ostream &stream, transient_container_type const &ct) const = 0;

		metadata_format *get_metadata() const final { return m_metadata; }

		// Check whether the sample has a value for this genotype field.
		// (The field could just be removed from FORMAT but instead the
		// specification allows MISSING value to be specified. See VCF 4.3 Section 1.6.2.)
		constexpr inline bool has_value(variant_sample_base const &sample) const;

		constexpr enum subfield_type subfield_type() const final { return subfield_type::GENOTYPE; }
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

/*
 * Copyright (c) 2019-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_SUBFIELD_INFO_FIELD_BASE_DECL_HH
#define LIBBIO_VCF_SUBFIELD_INFO_FIELD_BASE_DECL_HH

#include <cstddef>
#include <cstdint>
#include <libbio/utility.hh>
#include <libbio/vcf/constants.hh>
#include <libbio/vcf/metadata.hh>
#include <libbio/vcf/subfield/base.hh>
#include <libbio/vcf/variant/abstract_variant_decl.hh>
#include <libbio/vcf/variant/fwd.hh>
#include <map>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>


namespace libbio::vcf::detail {
	class metadata_setup_helper; // Fwd.
}


namespace libbio::vcf {

	class abstract_variant;
	class metadata_info;


	// Base class for info field descriptions.
	class info_field_base :	public subfield_base
	{
		friend class reader;
		friend class detail::metadata_setup_helper;

		template <typename, typename>
		friend class formatted_variant;

	protected:
		template <bool B>
		using container_tpl = variant_base_t <B>;

	public:
		typedef variant_base_t <false>	container_type;
		typedef variant_base_t <true>	transient_container_type;

		template <enum metadata_value_type M, bool B>
		using typed_field_type = typed_info_field_t <M, B>;

	protected:
		metadata_info	*m_metadata{};

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
		constexpr metadata_info *get_metadata() const final { return m_metadata; }

		// Output the field contents to a stream.
		// In case of info_field, the value has to be present in the variant.
		virtual void output_vcf_value(std::ostream &stream, container_type const &ct) const = 0;
		virtual void output_vcf_value(std::ostream &stream, transient_container_type const &ct) const = 0;

		// Output the given separator and the field contents to a stream if the value in present in the variant.
		template <typename t_string, typename t_format_access>
		bool output_vcf_value(
			std::ostream &stream,
			formatted_variant <t_string, t_format_access> const &var,
			char const *sep
		) const;

		// Check whether the variant has a value for this INFO field.
		constexpr inline bool has_value(abstract_variant const &var) const;

		constexpr enum subfield_type subfield_type() const final { return subfield_type::INFO; }
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

/*
 * Copyright (c) 2019-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_SUBFIELD_BASE_HH
#define LIBBIO_VCF_SUBFIELD_BASE_HH

#include <cstdint>
#include <libbio/vcf/constants.hh>
#include <libbio/vcf/subfield/utility/type_mapping.hh>
#include <libbio/vcf/variant/fwd.hh>


namespace libbio::vcf::detail {

	// Fwd.
	template <typename>
	struct typed_field_base;
}


namespace libbio::vcf {

	// Fwd.
	class metadata_base;
	class info_field_base;
	class genotype_field_base;

	typedef detail::typed_field_base <info_field_base>		typed_info_field_base;
	typedef detail::typed_field_base <genotype_field_base>	typed_genotype_field_base;

	template <metadata_value_type t_value_type, bool t_is_vector, typename t_base>
	class typed_field;

	// Aliases for easier castability.
	template <metadata_value_type t_value_type, bool t_is_vector>
	using typed_info_field_t = typed_field <t_value_type, t_is_vector, info_field_base>;

	template <metadata_value_type t_value_type, bool t_is_vector>
	using typed_genotype_field_t = typed_field <t_value_type, t_is_vector, genotype_field_base>;


	enum class subfield_type : std::uint8_t
	{
		UNKNOWN = 0,
		INFO,
		GENOTYPE
	};


	constexpr inline char const *subfield_type_name(subfield_type const st)
	{
		switch (st)
		{
			case subfield_type::UNKNOWN:	return "Unknown";
			case subfield_type::INFO:		return "Info";
			case subfield_type::GENOTYPE:	return "Genotype";
		}
	}


	// Base class and interface for field descriptions (specified by ##INFO, ##FORMAT).
	class subfield_base
	{
		friend class metadata_format;
		friend class metadata_formatted_field;

	public:
		enum { INVALID_OFFSET = UINT16_MAX };

	protected:
		std::uint16_t		m_offset{INVALID_OFFSET};	// Offset of this field in the memory block.

	public:
		virtual ~subfield_base() {}

		// Value type according to the VCF header.
		virtual enum metadata_value_type metadata_value_type() const = 0;

		// Number of items in this field according to the VCF header.
		virtual std::int32_t number() const = 0;

		constexpr bool value_type_is_vector() const { return value_count_corresponds_to_vector(number()); }

		// Get the metadata pointer.
		virtual metadata_base *get_metadata() const = 0;

		// Whether the field uses the enumerated VCF type mapping.
		constexpr virtual bool uses_vcf_type_mapping() const { return false; }

		// Subfield type (info or genotype)
		constexpr virtual enum subfield_type subfield_type() const { return subfield_type::UNKNOWN; }

	protected:
		constexpr std::uint16_t get_offset() const { return m_offset; }
		constexpr void set_offset(std::uint16_t offset) { m_offset = offset; }

		// Alignment of this field.
		virtual std::uint16_t alignment() const = 0;

		// Size of this field in bytes.
		virtual std::uint16_t byte_size() const = 0;

		// Create a clone of this object.
		virtual subfield_base *clone() const = 0;
	};
}

#endif

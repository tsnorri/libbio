/*
 * Copyright (c) 2019-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_SUBFIELD_DECL_HH
#define LIBBIO_VCF_SUBFIELD_DECL_HH

#include <cstdint>
#include <libbio/vcf/constants.hh>
#include <type_traits> // std::integral_constant


namespace libbio::vcf::detail {

	template <metadata_value_type t_metadata_value_type>
	struct field_default_count : public std::integral_constant <std::int32_t, 1> {};

	template <>
	struct field_default_count <metadata_value_type::FLAG> : public std::integral_constant <std::int32_t, 0> {};

	template <metadata_value_type t_metadata_value_type>
	constexpr inline auto const field_default_count_v{field_default_count <t_metadata_value_type>::value};
}


namespace libbio::vcf {

	// Fwd.
	template <typename t_base>
	class generic_field;

	template <metadata_value_type t_metadata_value_type, std::int32_t t_number>
	class generic_info_field_base;

	template <metadata_value_type t_metadata_value_type, std::int32_t t_number>
	class generic_genotype_field_base;

	template <metadata_value_type t_metadata_value_type, std::int32_t t_number = detail::field_default_count_v <t_metadata_value_type>>
	using info_field = generic_field <generic_info_field_base <t_metadata_value_type, t_number>>;

	template <metadata_value_type t_metadata_value_type, std::int32_t t_number = detail::field_default_count_v <t_metadata_value_type>>
	using genotype_field = generic_field <generic_genotype_field_base <t_metadata_value_type, t_number>>;

	// Info fields.
	using info_field_aa			= info_field <metadata_value_type::STRING>;
	using info_field_ac			= info_field <metadata_value_type::INTEGER,		VCF_NUMBER_ONE_PER_ALTERNATE_ALLELE>;
	using info_field_ad			= info_field <metadata_value_type::INTEGER,		VCF_NUMBER_ONE_PER_ALLELE>;
	using info_field_adf		= info_field <metadata_value_type::INTEGER,		VCF_NUMBER_ONE_PER_ALLELE>;
	using info_field_adr		= info_field <metadata_value_type::INTEGER,		VCF_NUMBER_ONE_PER_ALLELE>;
	using info_field_af			= info_field <metadata_value_type::FLOAT,		VCF_NUMBER_ONE_PER_ALTERNATE_ALLELE>;
	using info_field_an			= info_field <metadata_value_type::INTEGER>;
	using info_field_bq			= info_field <metadata_value_type::FLOAT>;
	using info_field_cigar		= info_field <metadata_value_type::STRING,		VCF_NUMBER_ONE_PER_ALTERNATE_ALLELE>;
	using info_field_db			= info_field <metadata_value_type::FLAG>;
	using info_field_dp			= info_field <metadata_value_type::INTEGER>;
	using info_field_end		= info_field <metadata_value_type::INTEGER>;
	using info_field_h2			= info_field <metadata_value_type::FLAG>;
	using info_field_h3			= info_field <metadata_value_type::FLAG>;
	using info_field_mq			= info_field <metadata_value_type::FLOAT>;
	using info_field_mq0		= info_field <metadata_value_type::INTEGER>;
	using info_field_ns			= info_field <metadata_value_type::INTEGER>;
	using info_field_sb			= info_field <metadata_value_type::INTEGER,		4>;
	using info_field_somatic	= info_field <metadata_value_type::FLAG>;
	using info_field_validated	= info_field <metadata_value_type::FLAG>;
	using info_field_1000g		= info_field <metadata_value_type::FLAG>;

	// Genotype fields.
	using genotype_field_ad		= genotype_field <metadata_value_type::INTEGER,	VCF_NUMBER_ONE_PER_ALLELE>;
	using genotype_field_adf	= genotype_field <metadata_value_type::INTEGER,	VCF_NUMBER_ONE_PER_ALLELE>;
	using genotype_field_adr	= genotype_field <metadata_value_type::INTEGER,	VCF_NUMBER_ONE_PER_ALLELE>;
	using genotype_field_dp		= genotype_field <metadata_value_type::INTEGER>;
	using genotype_field_ec		= genotype_field <metadata_value_type::INTEGER,	VCF_NUMBER_ONE_PER_ALTERNATE_ALLELE>;
	using genotype_field_ft		= genotype_field <metadata_value_type::STRING>;
	using genotype_field_gl		= genotype_field <metadata_value_type::FLOAT,	VCF_NUMBER_ONE_PER_GENOTYPE>;
	using genotype_field_gp		= genotype_field <metadata_value_type::FLOAT,	VCF_NUMBER_ONE_PER_GENOTYPE>;
	using genotype_field_gq		= genotype_field <metadata_value_type::INTEGER>;
	using genotype_field_hq		= genotype_field <metadata_value_type::INTEGER,	2>;
	using genotype_field_mq		= genotype_field <metadata_value_type::INTEGER>;
	using genotype_field_pl		= genotype_field <metadata_value_type::INTEGER,	VCF_NUMBER_ONE_PER_GENOTYPE>;
	using genotype_field_pq		= genotype_field <metadata_value_type::INTEGER>;
	using genotype_field_ps		= genotype_field <metadata_value_type::INTEGER>;
}

#endif

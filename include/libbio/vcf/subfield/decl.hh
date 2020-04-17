/*
 * Copyright (c) 2019-2020 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_SUBFIELD_DECL_HH
#define LIBBIO_VCF_SUBFIELD_DECL_HH

#include <libbio/types.hh>


namespace libbio::vcf {
	
	// Fwd.
	template <typename t_base>
	class generic_field;
	
	template <std::int32_t t_number, metadata_value_type t_metadata_value_type>
	class generic_info_field_base;
	
	template <std::int32_t t_number, metadata_value_type t_metadata_value_type>
	class generic_genotype_field_base;
	
	template <std::int32_t t_number, metadata_value_type t_metadata_value_type>
	using info_field = generic_field <generic_info_field_base <t_number, t_metadata_value_type>>;

	template <std::int32_t t_number, metadata_value_type t_metadata_value_type>
	using genotype_field = generic_field <generic_genotype_field_base <t_number, t_metadata_value_type>>;
	
	// Info fields.
	using info_field_aa			= info_field <1,										metadata_value_type::STRING>;
	using info_field_ac			= info_field <VCF_NUMBER_ONE_PER_ALTERNATE_ALLELE,		metadata_value_type::INTEGER>;
	using info_field_ad			= info_field <VCF_NUMBER_ONE_PER_ALLELE,				metadata_value_type::INTEGER>;
	using info_field_adf		= info_field <VCF_NUMBER_ONE_PER_ALLELE,				metadata_value_type::INTEGER>;
	using info_field_adr		= info_field <VCF_NUMBER_ONE_PER_ALLELE,				metadata_value_type::INTEGER>;
	using info_field_af			= info_field <VCF_NUMBER_ONE_PER_ALTERNATE_ALLELE,		metadata_value_type::FLOAT>;
	using info_field_an			= info_field <1,										metadata_value_type::INTEGER>;
	using info_field_bq			= info_field <1,										metadata_value_type::FLOAT>;
	using info_field_cigar		= info_field <VCF_NUMBER_ONE_PER_ALTERNATE_ALLELE,		metadata_value_type::STRING>;
	using info_field_db			= info_field <0,										metadata_value_type::FLAG>;
	using info_field_dp			= info_field <1,										metadata_value_type::INTEGER>;
	using info_field_end		= info_field <1,										metadata_value_type::INTEGER>;
	using info_field_h2			= info_field <0,										metadata_value_type::FLAG>;
	using info_field_h3			= info_field <0,										metadata_value_type::FLAG>;
	using info_field_mq			= info_field <1,										metadata_value_type::FLOAT>;
	using info_field_mq0		= info_field <1,										metadata_value_type::INTEGER>;
	using info_field_ns			= info_field <1,										metadata_value_type::INTEGER>;
	using info_field_sb			= info_field <4,										metadata_value_type::INTEGER>;
	using info_field_somatic	= info_field <0,										metadata_value_type::FLAG>;
	using info_field_validated	= info_field <0,										metadata_value_type::FLAG>;
	using info_field_1000g		= info_field <0,										metadata_value_type::FLAG>;
	
	// Genotype fields.
	using genotype_field_ad		= genotype_field <VCF_NUMBER_ONE_PER_ALLELE,			metadata_value_type::INTEGER>;
	using genotype_field_adf	= genotype_field <VCF_NUMBER_ONE_PER_ALLELE,			metadata_value_type::INTEGER>;
	using genotype_field_adr	= genotype_field <VCF_NUMBER_ONE_PER_ALLELE,			metadata_value_type::INTEGER>;
	using genotype_field_dp		= genotype_field <1,									metadata_value_type::INTEGER>;
	using genotype_field_ec		= genotype_field <VCF_NUMBER_ONE_PER_ALTERNATE_ALLELE,	metadata_value_type::INTEGER>;
	using genotype_field_ft		= genotype_field <1,									metadata_value_type::STRING>;
	using genotype_field_gl		= genotype_field <VCF_NUMBER_ONE_PER_GENOTYPE,			metadata_value_type::FLOAT>;
	using genotype_field_gp		= genotype_field <VCF_NUMBER_ONE_PER_GENOTYPE,			metadata_value_type::FLOAT>;
	using genotype_field_gq		= genotype_field <1,									metadata_value_type::INTEGER>;
	using genotype_field_hq		= genotype_field <2,									metadata_value_type::INTEGER>;
	using genotype_field_mq		= genotype_field <1,									metadata_value_type::INTEGER>;
	using genotype_field_pl		= genotype_field <VCF_NUMBER_ONE_PER_GENOTYPE,			metadata_value_type::INTEGER>;
	using genotype_field_pq		= genotype_field <1,									metadata_value_type::INTEGER>;
	using genotype_field_ps		= genotype_field <1,									metadata_value_type::INTEGER>;
}

#endif

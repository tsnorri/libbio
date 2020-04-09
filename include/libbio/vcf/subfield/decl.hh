/*
 * Copyright (c) 2019-2020 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_SUBFIELD_DECL_HH
#define LIBBIO_VCF_SUBFIELD_DECL_HH

#include <libbio/types.hh>


namespace libbio {
	
	// Fwd.
	template <typename t_base>
	class vcf_generic_field;
	
	template <std::int32_t t_number, vcf_metadata_value_type t_metadata_value_type>
	class vcf_generic_info_field_base;
	
	template <std::int32_t t_number, vcf_metadata_value_type t_metadata_value_type>
	class vcf_generic_genotype_field_base;
	
	template <std::int32_t t_number, vcf_metadata_value_type t_metadata_value_type>
	using vcf_info_field = vcf_generic_field <vcf_generic_info_field_base <t_number, t_metadata_value_type>>;

	template <std::int32_t t_number, vcf_metadata_value_type t_metadata_value_type>
	using vcf_genotype_field = vcf_generic_field <vcf_generic_genotype_field_base <t_number, t_metadata_value_type>>;
	
	// Info fields.
	using vcf_info_field_aa			= vcf_info_field <1,										vcf_metadata_value_type::STRING>;
	using vcf_info_field_ac			= vcf_info_field <VCF_NUMBER_ONE_PER_ALTERNATE_ALLELE,		vcf_metadata_value_type::INTEGER>;
	using vcf_info_field_ad			= vcf_info_field <VCF_NUMBER_ONE_PER_ALLELE,				vcf_metadata_value_type::INTEGER>;
	using vcf_info_field_adf		= vcf_info_field <VCF_NUMBER_ONE_PER_ALLELE,				vcf_metadata_value_type::INTEGER>;
	using vcf_info_field_adr		= vcf_info_field <VCF_NUMBER_ONE_PER_ALLELE,				vcf_metadata_value_type::INTEGER>;
	using vcf_info_field_af			= vcf_info_field <VCF_NUMBER_ONE_PER_ALTERNATE_ALLELE,		vcf_metadata_value_type::FLOAT>;
	using vcf_info_field_an			= vcf_info_field <1,										vcf_metadata_value_type::INTEGER>;
	using vcf_info_field_bq			= vcf_info_field <1,										vcf_metadata_value_type::FLOAT>;
	using vcf_info_field_cigar		= vcf_info_field <VCF_NUMBER_ONE_PER_ALTERNATE_ALLELE,		vcf_metadata_value_type::STRING>;
	using vcf_info_field_db			= vcf_info_field <0,										vcf_metadata_value_type::FLAG>;
	using vcf_info_field_dp			= vcf_info_field <1,										vcf_metadata_value_type::INTEGER>;
	using vcf_info_field_end		= vcf_info_field <1,										vcf_metadata_value_type::INTEGER>;
	using vcf_info_field_h2			= vcf_info_field <0,										vcf_metadata_value_type::FLAG>;
	using vcf_info_field_h3			= vcf_info_field <0,										vcf_metadata_value_type::FLAG>;
	using vcf_info_field_mq			= vcf_info_field <1,										vcf_metadata_value_type::FLOAT>;
	using vcf_info_field_mq0		= vcf_info_field <1,										vcf_metadata_value_type::INTEGER>;
	using vcf_info_field_ns			= vcf_info_field <1,										vcf_metadata_value_type::INTEGER>;
	using vcf_info_field_sb			= vcf_info_field <4,										vcf_metadata_value_type::INTEGER>;
	using vcf_info_field_somatic	= vcf_info_field <0,										vcf_metadata_value_type::FLAG>;
	using vcf_info_field_validated	= vcf_info_field <0,										vcf_metadata_value_type::FLAG>;
	using vcf_info_field_1000g		= vcf_info_field <0,										vcf_metadata_value_type::FLAG>;
	
	// Genotype fields.
	using vcf_genotype_field_ad		= vcf_genotype_field <VCF_NUMBER_ONE_PER_ALLELE,			vcf_metadata_value_type::INTEGER>;
	using vcf_genotype_field_adf	= vcf_genotype_field <VCF_NUMBER_ONE_PER_ALLELE,			vcf_metadata_value_type::INTEGER>;
	using vcf_genotype_field_adr	= vcf_genotype_field <VCF_NUMBER_ONE_PER_ALLELE,			vcf_metadata_value_type::INTEGER>;
	using vcf_genotype_field_dp		= vcf_genotype_field <1,									vcf_metadata_value_type::INTEGER>;
	using vcf_genotype_field_ec		= vcf_genotype_field <VCF_NUMBER_ONE_PER_ALTERNATE_ALLELE,	vcf_metadata_value_type::INTEGER>;
	using vcf_genotype_field_ft		= vcf_genotype_field <1,									vcf_metadata_value_type::STRING>;
	using vcf_genotype_field_gl		= vcf_genotype_field <VCF_NUMBER_ONE_PER_GENOTYPE,			vcf_metadata_value_type::FLOAT>;
	using vcf_genotype_field_gp		= vcf_genotype_field <VCF_NUMBER_ONE_PER_GENOTYPE,			vcf_metadata_value_type::FLOAT>;
	using vcf_genotype_field_gq		= vcf_genotype_field <1,									vcf_metadata_value_type::INTEGER>;
	using vcf_genotype_field_hq		= vcf_genotype_field <2,									vcf_metadata_value_type::INTEGER>;
	using vcf_genotype_field_mq		= vcf_genotype_field <1,									vcf_metadata_value_type::INTEGER>;
	using vcf_genotype_field_pl		= vcf_genotype_field <VCF_NUMBER_ONE_PER_GENOTYPE,			vcf_metadata_value_type::INTEGER>;
	using vcf_genotype_field_pq		= vcf_genotype_field <1,									vcf_metadata_value_type::INTEGER>;
	using vcf_genotype_field_ps		= vcf_genotype_field <1,									vcf_metadata_value_type::INTEGER>;
}

#endif

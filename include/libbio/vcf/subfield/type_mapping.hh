/*
 * Copyright (c) 2020 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_SUBFIELD_TYPE_MAPPING_HH
#define LIBBIO_VCF_SUBFIELD_TYPE_MAPPING_HH

#include <libbio/types.hh>


namespace libbio {

	// Map VCF types to C++ types. Use a byte for FLAG since there are no means for using bit fields right now.
	template <vcf_metadata_value_type t_metadata_value_type, bool t_is_transient> struct vcf_field_type_mapping {};
	
	template <bool t_is_transient> struct vcf_field_type_mapping <vcf_metadata_value_type::FLAG, t_is_transient>	{ typedef std::uint8_t type; };
	template <bool t_is_transient> struct vcf_field_type_mapping <vcf_metadata_value_type::INTEGER, t_is_transient>	{ typedef std::int32_t type; };
	template <bool t_is_transient> struct vcf_field_type_mapping <vcf_metadata_value_type::FLOAT, t_is_transient>	{ typedef float type; };
	template <> struct vcf_field_type_mapping <vcf_metadata_value_type::STRING, false>								{ typedef std::string type; };
	template <> struct vcf_field_type_mapping <vcf_metadata_value_type::CHARACTER, false>							{ typedef std::string type; };
	template <> struct vcf_field_type_mapping <vcf_metadata_value_type::STRING, true>								{ typedef std::string_view type; };
	template <> struct vcf_field_type_mapping <vcf_metadata_value_type::CHARACTER, true>							{ typedef std::string_view type; };
	
	template <vcf_metadata_value_type t_metadata_value_type, bool t_is_transient>
	using vcf_field_type_mapping_t = typename vcf_field_type_mapping <t_metadata_value_type, t_is_transient>::type;
	
	
	// The actual value types. (Non-)specialization for vector types.
	template <vcf_metadata_value_type t_metadata_value_type, bool t_is_vector, bool t_is_transient>
	struct vcf_value_type_mapping
	{
		typedef std::vector <vcf_field_type_mapping_t <t_metadata_value_type, t_is_transient>> type;
	};
	
	// Partial specialization for scalar types.
	template <vcf_metadata_value_type t_metadata_value_type, bool t_is_transient>
	struct vcf_value_type_mapping <t_metadata_value_type, false, t_is_transient>
	{
		typedef vcf_field_type_mapping_t <t_metadata_value_type, t_is_transient> type;
	};
	
	// Specialization for FLAG (only scalar).
	template <bool t_is_vector, bool t_is_transient>
	struct vcf_value_type_mapping <vcf_metadata_value_type::FLAG, t_is_vector, t_is_transient> {};
	
	template <bool t_is_transient>
	struct vcf_value_type_mapping <vcf_metadata_value_type::FLAG, false, t_is_transient>
	{
		typedef vcf_field_type_mapping_t <vcf_metadata_value_type::FLAG, t_is_transient> type;
	};
	
	// Helper template.
	template <vcf_metadata_value_type t_metadata_value_type, bool t_is_vector, bool t_is_transient>
	using vcf_value_type_mapping_t = typename vcf_value_type_mapping <t_metadata_value_type, t_is_vector, t_is_transient>::type;
	
	
	constexpr inline bool vcf_value_count_corresponds_to_vector(std::int32_t number) { return (1 < number || number < 0); }
}

#endif

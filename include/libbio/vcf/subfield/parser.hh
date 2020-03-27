/*
 * Copyright (c) 2019-2020 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_SUBFIELD_PARSER_HH
#define LIBBIO_VCF_SUBFIELD_PARSER_HH

#include <libbio/vcf/subfield/access.hh>


namespace libbio {

	// VCF value parser. The main parser handles vectors, so this class only needs to handle single values.
	// Since the value type is specified in the headers, making the parser stateful enough to parse the values
	// consistently would be more difficult than doing the parsing here.
	// Non-specialization intentionally left blank.
	template <vcf_metadata_value_type t_metadata_value_type>
	struct vcf_subfield_parser {};
	
	struct vcf_subfield_parser_base { static constexpr bool type_needs_parsing() { return true; } };
	
	// Specializations for the different value types.
	template <> struct vcf_subfield_parser <vcf_metadata_value_type::INTEGER> : public vcf_subfield_parser_base
	{
		typedef vcf_field_type_mapping_t <vcf_metadata_value_type::INTEGER, true>	value_type;
		static bool parse(std::string_view const &sv, value_type &dst);
	};
	
	template <> struct vcf_subfield_parser <vcf_metadata_value_type::FLOAT> : public vcf_subfield_parser_base
	{
		typedef vcf_field_type_mapping_t <vcf_metadata_value_type::FLOAT, true>	value_type;
		static bool parse(std::string_view const &sv, value_type &dst);
	};
	
	template <> struct vcf_subfield_parser <vcf_metadata_value_type::STRING> : public vcf_subfield_parser_base
	{
		static constexpr bool type_needs_parsing() { return false; }
	};
	
	template <> struct vcf_subfield_parser <vcf_metadata_value_type::CHARACTER> : public vcf_subfield_parser_base
	{
		static constexpr bool type_needs_parsing() { return false; }
	};
	
	
	// Helper for implementing parse_and_assign(). Vectors are handled here.
	template <std::int32_t t_number, vcf_metadata_value_type t_metadata_value_type>
	class vcf_generic_field_parser
	{
	protected:
		typedef vcf_subfield_access <t_number, t_metadata_value_type, true>	field_access;
		
	protected:
		bool parse_and_assign(std::string_view const &sv, std::byte *mem) const
		{
			// mem needs to include the offset.
			if constexpr (t_metadata_value_type == vcf_metadata_value_type::FLAG)
				libbio_fail("parse_and_assign should not be called for FLAG type fields");
			else
			{
				// Apparently VCF 4.3 specification (Section 1.6.2 Genotype fields)
				// only allows the whole field to be marked as MISSING, except for
				// GT fields.
				if ("." == sv)
					return false;
				
				typedef vcf_subfield_parser <t_metadata_value_type> parser_type;
				if constexpr (parser_type::type_needs_parsing())
				{
					std::size_t start_pos(0);
					while (true)
					{
						typename parser_type::value_type value{};
						auto const end_pos(sv.find_first_of(',', start_pos));
						auto const length(end_pos == std::string_view::npos ? end_pos : end_pos - start_pos);
						auto const sv_part(sv.substr(start_pos, length)); // npos is valid for end_pos.
						
						parser_type::parse(sv_part, value);
						field_access::add_value(mem, value);
						
						if (std::string_view::npos == end_pos)
							break;
						
						start_pos = 1 + end_pos;
					}
				}
				else
				{
					field_access::add_value(mem, sv);
				}
			}
			return true;
		}
	};
	
	// Non-specialization for values with zero elements.
	template <vcf_metadata_value_type t_metadata_value_type>
	struct vcf_generic_field_parser <0, t_metadata_value_type>
	{
		bool parse_and_assign(std::string_view const &sv, std::byte *mem) const
		{
			libbio_fail("parse_and_assign should not be called for FLAG type fields");
			return false;
		}
	};
	
	// Specialization for scalar values.
	template <vcf_metadata_value_type t_metadata_value_type>
	class vcf_generic_field_parser <1, t_metadata_value_type>
	{
	protected:
		typedef vcf_subfield_access <1, t_metadata_value_type, true>	field_access;
		
	protected:
		bool parse_and_assign(std::string_view const &sv, std::byte *mem) const
		{
			// mem needs to include the offset.
			if constexpr (t_metadata_value_type == vcf_metadata_value_type::FLAG)
				libbio_fail("parse_and_assign should not be called for FLAG type fields");
			else
			{
				typedef vcf_subfield_parser <t_metadata_value_type> parser_type;
				if ("." == sv)
					return false;
				
				if constexpr (parser_type::type_needs_parsing())
				{
					typename parser_type::value_type value{};
					parser_type::parse(sv, value);
					field_access::add_value(mem, value);
				}
				else
				{
					field_access::add_value(mem, sv);
				}
			}
			
			return true;
		}
	};
}

#endif

/*
 * Copyright (c) 2019-2022 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_SUBFIELD_PARSER_HH
#define LIBBIO_VCF_SUBFIELD_PARSER_HH

#include <libbio/vcf/subfield/utility/access.hh>


namespace libbio::vcf {

	// VCF value parser. The main parser handles vectors, so this class only needs to handle single values.
	// Since the value type is specified in the headers, making the parser stateful enough to parse the values
	// consistently would be more difficult than doing the parsing here.
	// Non-specialization intentionally left blank.
	template <metadata_value_type t_metadata_value_type>
	struct subfield_parser {};
	
	struct subfield_parser_base { static constexpr bool type_needs_parsing() { return true; } };
	
	// Specializations for the different value types.
	template <> struct subfield_parser <metadata_value_type::INTEGER> final : public subfield_parser_base
	{
		typedef field_type_mapping_t <metadata_value_type::INTEGER, true>	value_type;
		static bool parse(std::string_view const &sv, value_type &dst, metadata_formatted_field const *field);
	};
	
	template <> struct subfield_parser <metadata_value_type::FLOAT> final : public subfield_parser_base
	{
		typedef field_type_mapping_t <metadata_value_type::FLOAT, true>	value_type;
		static bool parse(std::string_view const &sv, value_type &dst, metadata_formatted_field const *field);
	};
	
	template <> struct subfield_parser <metadata_value_type::STRING> final : public subfield_parser_base
	{
		static constexpr bool type_needs_parsing() { return false; }
	};
	
	template <> struct subfield_parser <metadata_value_type::CHARACTER> final : public subfield_parser_base
	{
		static constexpr bool type_needs_parsing() { return false; }
	};
	
	
	// Helper for implementing parse_and_assign(). Vectors are handled here.
	template <metadata_value_type t_metadata_value_type, std::int32_t t_number>
	class generic_field_parser
	{
	protected:
		typedef subfield_access <t_metadata_value_type, t_number, true>	field_access;
		
	protected:
		bool parse_and_assign(std::string_view const &sv, std::byte *mem, metadata_formatted_field const *field) const
		{
			// mem needs to include the offset.
			if constexpr (t_metadata_value_type == metadata_value_type::FLAG)
				libbio_fail("parse_and_assign should not be called for FLAG type fields");
			else
			{
				// Apparently VCF 4.3 specification (Section 1.6.2 Genotype fields)
				// only allows the whole field to be marked as MISSING, except for
				// GT fields.
				if ("." == sv)
					return false;
				
				typedef subfield_parser <t_metadata_value_type> parser_type;
				if constexpr (parser_type::type_needs_parsing())
				{
					std::size_t start_pos(0);
					while (true)
					{
						typename parser_type::value_type value{};
						auto const end_pos(sv.find_first_of(',', start_pos));
						auto const length(end_pos == std::string_view::npos ? end_pos : end_pos - start_pos);
						auto const sv_part(sv.substr(start_pos, length)); // npos is valid for end_pos.
						
						parser_type::parse(sv_part, value, field);
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
	template <metadata_value_type t_metadata_value_type>
	struct generic_field_parser <t_metadata_value_type, 0>
	{
		bool parse_and_assign(std::string_view const &sv, std::byte *mem, metadata_formatted_field const *field) const
		{
			libbio_fail("parse_and_assign should not be called for FLAG type fields");
			return false;
		}
	};
	
	// Specialization for scalar values.
	template <metadata_value_type t_metadata_value_type>
	class generic_field_parser <t_metadata_value_type, 1>
	{
	protected:
		typedef subfield_access <t_metadata_value_type, 1, true>	field_access; // May be transient b.c. used only for parsing.
		
	protected:
		bool parse_and_assign(std::string_view const &sv, std::byte *mem, metadata_formatted_field const *field) const
		{
			// mem needs to include the offset.
			if constexpr (t_metadata_value_type == metadata_value_type::FLAG)
				libbio_fail("parse_and_assign should not be called for FLAG type fields");
			else
			{
				typedef subfield_parser <t_metadata_value_type> parser_type;
				if ("." == sv)
					return false;
				
				if constexpr (parser_type::type_needs_parsing())
				{
					typename parser_type::value_type value{};
					parser_type::parse(sv, value, field);
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

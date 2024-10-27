/*
 * Copyright (c) 2017-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <cstddef>
#include <cstdint>
#include <libbio/assert.hh>
#include <libbio/vcf/constants.hh>
#include <libbio/vcf/metadata.hh>
#include <libbio/vcf/variant.hh>
#include <libbio/vcf/vcf_reader_decl.hh>
#include <string_view>
#include "vcf_reader_private.hh"


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"


%% machine vcf_header_parser;
%% write data;


#define BEGIN_METADATA(CLASS, IDX)	do { \
										current_metadata_ptr = &current_metadata.emplace <CLASS>(); \
										current_metadata_ptr->m_header_index	= counters[0]++; \
										current_metadata_ptr->m_index			= counters[1 + IDX]++; \
									} while (false)
#define END_METADATA(CLASS)			do { /* add_metadata() takes an rvalue reference. */ \
										m_metadata.add_metadata(std::move(std::get <CLASS>(current_metadata))); \
										current_metadata_ptr = nullptr; \
									} while (false)


namespace {

	enum header_type {
		INFO = 0,
		FILTER,
		FORMAT,
		ALT,
		ASSEMBLY,
		CONTIG,
		HEADER_COUNT
	};
}


namespace libbio::vcf {

	void reader::read_header()
	{
		typedef metadata_base	meta_t;

		bool					should_continue(false);

		metadata_record_var		current_metadata;
		metadata_base			*current_metadata_ptr{};

		char const				*buffer_start(nullptr);
		char const				*start(nullptr);				// Current string start.
		char const				*line_start(nullptr);			// Current line start.
		std::uint16_t			counters[1 + HEADER_COUNT]{};

		std::size_t				sample_name_idx(1);
		std::int64_t			integer(0);						// Currently read from the input.
		bool					integer_is_negative(false);
		int						cs(0);

		%%{
			machine vcf_header_parser;

			variable p		m_fsm.p;
			variable pe		m_fsm.pe;
			variable eof	m_fsm.eof;

			action error {
				report_unexpected_character(fpc, fpc - line_start, fcurs, true);
			}

			action start_string {
				start = fpc;
			}

			action start_integer {
				integer = 0;
			}

			action update_integer {
				// Shift and add the current number.
				integer *= 10;
				integer += fc - '0';
			}

			action finish_integer {
				if (integer_is_negative)
					integer *= -1;
				integer_is_negative = false;
			}

			action meta_record_info			{ BEGIN_METADATA(metadata_info,		INFO); }
			action meta_record_filter		{ BEGIN_METADATA(metadata_filter,	FILTER); }
			action meta_record_format		{ BEGIN_METADATA(metadata_format,	FORMAT); }
			action meta_record_alt			{ BEGIN_METADATA(metadata_alt,		ALT); }
			action meta_record_assembly		{ BEGIN_METADATA(metadata_assembly,	ASSEMBLY); }
			action meta_record_contig		{ BEGIN_METADATA(metadata_contig,	CONTIG); }

			action meta_record_info_end		{ END_METADATA(metadata_info); }
			action meta_record_filter_end	{ END_METADATA(metadata_filter); }
			action meta_record_format_end	{ END_METADATA(metadata_format); }
			action meta_record_alt_end		{ END_METADATA(metadata_alt); }
			action meta_record_contig_end	{ END_METADATA(metadata_contig); }
			action meta_record_assembly_end	{ END_METADATA(metadata_assembly); }

			action end_sample_name {
				std::string_view const sample_name(start, fpc - start);
				auto const res(m_sample_indices_by_name.emplace(sample_name, sample_name_idx));
				libbio_always_assert_msg(res.second, "Duplicate sample name");
				m_sample_names_by_index.emplace_back(sample_name);
				++sample_name_idx;
			}

			action has_samples {
				m_has_samples = true;
			}

			action goto_header_sub			{ fgoto header_sub; }

			action end_line					{ line_start = fpc; ++m_lineno; }

			integer							= ("+" | ("-" >{ integer_is_negative = true; }))? ([0-9]+) >(start_integer) $(update_integer) %(finish_integer);

			header_sub_nl					= "\n" >{ ++m_lineno; line_start = fpc; fgoto header_sub; };

			# VCF 4.2 specification calls these (##INFO, ##ALT etc.) fields. Here, subfields (ID, Description, etc.)
			# are called fields and the fields are called records, instead.
			meta_literal_char				= [^,>\n];
			meta_literal					= meta_literal_char+;

			# ID field.
			meta_field_id					= "ID=" (meta_literal) >(start_string) %{ HANDLE_STRING_END_METADATA(&meta_t::set_id); };

			# Number field.
			meta_field_number				= "Number=" (
				(integer	%{ HANDLE_INTEGER_END_METADATA(&meta_t::set_number); }			) |
				('A'		%{ CURRENT_METADATA.set_number_one_per_alternate_allele(); }	) |
				('R'		%{ CURRENT_METADATA.set_number_one_per_allele(); }				) |
				('G'		%{ CURRENT_METADATA.set_number_one_per_genotype(); }			) |
				('.'		%{ CURRENT_METADATA.set_number_unknown(); }						));

			# Description field.
			# The VCF 4.2 spec apparently has a bug in Section 1.2.5 Alternative allele field format.
			# The other fields that may have a description value as well as the examples of ALT have the description
			# surrounded in quotes.
			meta_field_description			= 'Description="'
				([^"]*) >(start_string) %{ HANDLE_STRING_END_METADATA(&meta_t::set_description); }
				'"';

			# Source field.
			meta_field_source				= 'Source="'
				([^"]*) >(start_string) %{ HANDLE_STRING_END_METADATA(&meta_t::set_source); }
				'"';

			# Version field.
			meta_field_version				= 'Version="'
				([^"]*) >(start_string) %{ HANDLE_STRING_END_METADATA(&meta_t::set_version); }
				'"';

			# URL field.
			meta_field_url					= 'URL=' (meta_literal)
				>(start_string)
				%{ HANDLE_STRING_END_METADATA(&meta_t::set_url); };

			# length field.
			meta_field_length				= "length=" integer %{ HANDLE_INTEGER_END_METADATA(&meta_t::set_length); };

			# assembly field.
			meta_field_assembly				= "assembly=" (meta_literal)
				>(start_string)
				%{ HANDLE_STRING_END_METADATA(&meta_t::set_assembly); };

			# md5 field.
			meta_field_md5					= "md5=" (meta_literal)
				>(start_string)
				%{ HANDLE_STRING_END_METADATA(&meta_t::set_md5); };

			# taxonomy field.
			meta_field_taxonomy				= "taxonomy=" (meta_literal)
				>(start_string)
				%{ HANDLE_STRING_END_METADATA(&meta_t::set_taxonomy); };

			# species field.
			meta_field_species				= 'species="'
				([^"]*) >(start_string) %{ HANDLE_STRING_END_METADATA(&meta_t::set_species); }
				'"';

			# Type field.
			meta_type_integer				= "Integer"		%{ CURRENT_METADATA.set_value_type(metadata_value_type::INTEGER);	};
			meta_type_float					= "Float"		%{ CURRENT_METADATA.set_value_type(metadata_value_type::FLOAT);		};
			meta_type_character				= "Character"	%{ CURRENT_METADATA.set_value_type(metadata_value_type::CHARACTER);	};
			meta_type_string				= "String"		%{ CURRENT_METADATA.set_value_type(metadata_value_type::STRING);	};
			meta_type_flag					= "Flag"		%{ CURRENT_METADATA.set_value_type(metadata_value_type::FLAG);		};
			meta_type						=	meta_type_integer |
												meta_type_float |
												meta_type_character |
												meta_type_string |
												meta_type_flag;
			meta_field_type					= "Type=" (meta_type);

			# generic field.
			meta_field_generic				= meta_literal+ '=' meta_literal+;

			# INFO record.
			meta_record_info_field			=	meta_field_id |
												meta_field_number |
												meta_field_type |
												meta_field_description |
												meta_field_source |
												meta_field_version;
			meta_record_info				:=
				(
					"=<"
					(meta_record_info_field (',' meta_record_info_field)*)
					">"
				) >(meta_record_info) %(meta_record_info_end) header_sub_nl $err(error);

			# FILTER record.
			meta_record_filter_field		= meta_field_id | meta_field_description;
			meta_record_filter				:=
				(
					"=<"
					(meta_record_filter_field (',' meta_record_filter_field)*)
					">"
				) >(meta_record_filter) %(meta_record_filter_end) header_sub_nl $err(error);

			# FORMAT record.
			meta_record_format_field		= meta_field_id | meta_field_number | meta_field_type | meta_field_description;
			meta_record_format				:=
				(
					"=<" (meta_record_format_field (',' meta_record_format_field)*)
					">"
				) >(meta_record_format) %(meta_record_format_end) header_sub_nl $err(error);

			# ALT record.
			meta_field_alt_id				=
				"ID="
				(
					(
						"DEL" |
						"INS" |
						"DUP" |
						"INV" |
						"CNV" |
						"DUP:TANDEM" |
						"DEL:ME" |
						"INS:ME" |
						((meta_literal_char - ':')+)		# At least the 1000G VCF files can have ALT types like CN0, CN1 etc. that are not mentioned in the newer specifications.
					) (':' (meta_literal_char - ':')+)*
				)
				>(start_string) %{ HANDLE_STRING_END_METADATA(&meta_t::set_id); };
			meta_record_alt_field			= meta_field_alt_id | meta_field_description;
			meta_record_alt					:=
				(
					"=<"
					(meta_record_alt_field (',' meta_record_alt_field)*)
					">"
				) >(meta_record_alt) %(meta_record_alt_end) header_sub_nl $err(error);

			# Assembly record.
			meta_record_assembly			:=
				(
					"=" ([^\n]+) >(start_string) %{ HANDLE_STRING_END_METADATA(&meta_t::set_assembly); }
				) >(meta_record_assembly) %(meta_record_assembly_end) header_sub_nl $err(error);

			# Contig record.
			# FIXME: are all the fields present?
			meta_record_contig_field		=
				meta_field_id |
				meta_field_length |
				meta_field_generic;
				#meta_field_assembly |
				#meta_field_md5 |
				#meta_field_species |
				#meta_field_taxonomy;
			meta_record_contig				:=
				(
					"=<"
					(meta_record_contig_field (',' meta_record_contig_field)*)
					">"
				) >(meta_record_contig) %(meta_record_contig_end) header_sub_nl $err(error);

			# Generic record for non-handled entries.
			meta_record_generic				= ([^\n]+) @ (meta_rec, 1) header_sub_nl;

			# Combine all of the records above.
			meta_record_name_or_generic		=	"INFO"		@(meta_rec, 2) @{ fgoto meta_record_info; }	|
												"FILTER"	@(meta_rec, 2) @{ fgoto meta_record_filter; }	|
												"FORMAT"	@(meta_rec, 2) @{ fgoto meta_record_format; }	|
												"ALT"		@(meta_rec, 2) @{ fgoto meta_record_alt; }		|
												"assembly"	@(meta_rec, 2) @{ fgoto meta_record_assembly; }	|
												"contig"	@(meta_rec, 2) @{ fgoto meta_record_contig; }	|
												meta_record_generic;
			meta							= "##" meta_record_name_or_generic;

			# Column headers.
			column_names					= "#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO";
			sample_name						= ([^\t\n]+) >(start_string) %(end_sample_name);
			sample_names					= sample_name ("\t" sample_name)*;
			column_header					= (column_names ("\tFORMAT\t" >(has_samples) sample_names)? "\n" >(end_line) @{ should_continue = false; fbreak; });

			fileformat						= ("##fileformat=VCFv4." [0-9]+ "\n" >(end_line));

			header_sub := (meta | column_header) $err(error);
			main := fileformat @(goto_header_sub) $err(error);

			write init;
		}%%

		do
		{
			fill_buffer();
			should_continue = true;
			buffer_start = m_fsm.p;
			%% write exec;
			m_variant_offset += m_fsm.p - buffer_start;
		} while (should_continue);

		// Post-process the metadata descriptions.
		m_delegate->vcf_reader_did_parse_metadata(*this);
		associate_metadata_with_field_descriptions();
		auto const [info_size, info_max_alignment] = assign_info_field_offsets();

		// Assign the genotype offsets after reading FORMAT.

		// stream now points to the first variant.
		m_input->set_first_variant_lineno(1 + m_lineno);

		// Instantiate a variant.
		transient_variant var(*this, sample_count(), info_size, info_max_alignment);
		using std::swap;
		swap(m_current_variant, var);
	}
}

#pragma GCC diagnostic pop

/*
 * Copyright (c) 2017-2019 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <libbio/assert.hh>
#include <libbio/vcf/variant.hh>
#include <libbio/vcf/vcf_reader.hh>
#include <libbio/vcf/vcf_subfield.hh>
#include "vcf_reader_private.hh"


%% machine vcf_parser;
%% write data;


namespace libbio {
	
	template <int t_continue, int t_break>
	int vcf_reader::check_max_field(vcf_field const field, int const target, parsing_delegate &cb)
	{
		if (field <= m_max_parsed_field)
			return target;
		
		skip_to_next_nl();
		if (!cb.handle_variant(*this, m_current_variant))
			return t_break;
		
		++m_counter;
		return t_continue;
	}
	
	
	bool vcf_reader::parse(parsing_delegate &cb)
	{
		typedef variant_tpl <std::string_view, transient_variant_format_access>	var_t;
		typedef variant_alt <std::string_view>									alt_t;
		
		bool retval(true);
		m_current_format->clear();
		m_current_format_vec.clear();
		
		vcf_info_field_interface		*current_info_field{};
		
		std::string						format_string;				// Current format as a string.
		char const						*start(nullptr);			// Current string start.
		char const						*line_start(nullptr);		// Current line start.
		std::int64_t					integer(0);					// Currently read from the input.
		std::size_t						sample_idx(0);				// Current sample idx (1-based).
		std::size_t						subfield_idx(0);			// Current index in multi-part fields.
		bool							integer_is_negative(false);
		bool							gt_is_phased(false);		// Is the current GT phased.
		bool							alt_is_complex(false);		// Is the current ALT “complex” (includes *).
		bool							info_is_flag(false);
		int								cs(0);
		
		// FIXME: add throwing EOF actions?
		// FIXME: add error actions to other machines than main?
		%%{
			machine vcf_parser;
			
			variable p		m_fsm.p;
			variable pe		m_fsm.pe;
			variable eof	m_fsm.eof;
			
			action start_string {
				start = fpc;
			}
			
			action start_alt {
				start = fpc;
				m_current_variant.m_alts.emplace_back();
			}
			
			action start_integer {
				integer = 0;
			}
			
			action update_integer {
				// Shift and add the current number.
				integer *= 10;
				integer += fc - '0';
			}
			
			action set_unknown_quality_value {
				integer = variant_base::UNKNOWN_QUALITY;
			}
			
			action end_info_key {
				// Find the listed key.
				std::string_view const key_sv(start, fpc - start);
				auto const it(m_info_fields.find(key_sv));
				if (m_info_fields.cend() == it)
					throw std::runtime_error("Unknown INFO key");
				current_info_field = it->second.get();
				if (!current_info_field->m_metadata)
					throw std::runtime_error("Found INFO field not declared in the VCF header");
			}
			
			action end_info_val {
				libbio_assert_eq(false, info_is_flag);
				libbio_assert(current_info_field);
				libbio_assert(current_info_field->m_metadata);
				if (!current_info_field->m_metadata->check_subfield_index(subfield_idx))
					throw std::runtime_error("More values in INFO field than declared");
				
				std::string_view const val_sv(start, fpc - start);
				current_info_field->parse_and_assign(val_sv, m_current_variant);
				++subfield_idx;
			}
			
			action start_info_values {
				// List of values for one key.
			}
			
			action end_info_values {
			}
			
			action info_not_flag {
				info_is_flag = false;
			}
			
			action info_flag {
				info_is_flag = true;
			}
			
			action end_info_kv {
				if (info_is_flag)
					current_info_field->assign_flag(m_current_variant);
				
				current_info_field = nullptr;
			}
			
			action start_info_kv {
				// Key and zero (FLAG) or more values.
				subfield_idx = 0;
			}
			
			action start_info {
				for (auto const *field_ptr : m_info_fields_in_headers)
					field_ptr->prepare(m_current_variant);
			}
			
			action end_info {
			}
			
			action end_format {
				std::string_view const new_format(start, fpc - start);
				if (new_format != format_string)
				{
					cb.reader_will_update_format(*this);
					deinitialize_variant_samples(m_current_variant, *m_current_format);
					
					format_string = new_format;
					parse_format(format_string);
					auto const [size, alignment] = assign_format_field_offsets();
					reserve_memory_for_samples_in_current_variant(size, alignment);
					
					initialize_variant_samples(m_current_variant, *m_current_format);
					cb.reader_did_update_format(*this);
				}
			}
			
			action end_sample_field {
				std::string_view const field_value(start, fpc - start);
				if (! (subfield_idx < m_current_format_vec.size()))
					throw std::runtime_error("More fields in current sample than specified in FORMAT");
				
				auto const &format_ptr(m_current_format_vec[subfield_idx]);
				auto &samples(m_current_variant.m_samples);
				libbio_always_assert_lt_msg(sample_idx, samples.size(), "Samples should be preallocated.");
				format_ptr->parse_and_assign(field_value, samples[sample_idx]);
				
				++subfield_idx;
			}
			
			action start_sample {
				subfield_idx = 0;
				
				for (auto const *field_ptr : m_current_format_vec)
				{
					auto &samples(m_current_variant.m_samples);
					libbio_always_assert_lt_msg(sample_idx, samples.size(), "Samples should be preallocated.");
					field_ptr->prepare(samples[sample_idx]);
				}
			}
			
			action end_sample {
				if (m_current_format_vec.size() != subfield_idx)
					throw std::runtime_error("Number of sample fields differs from FORMAT");
				
				++sample_idx;
			}
			
			action end_last_sample {
				if (m_sample_names.size() != sample_idx)
					throw std::runtime_error("Number of samples differs from VCF headers");
				
				if (!cb.handle_variant(*this, m_current_variant))
				{
					fhold;
					fbreak;
				}
				
				++m_counter;
				fgoto main;
			}
			
			action error {
				report_unexpected_character(fpc, fpc - line_start, fcurs);
			}
			
			tab			= '\t';
			
			id_string	= '<' alnum+ '>';
			
			chr			= graph;
			
			integer		= ("+" | ("-" >{ integer_is_negative = true; }))? ([0-9]+) >(start_integer) $(update_integer);
			
			chrom_id	= (chr+)
				>(start_string)
				%{ HANDLE_STRING_END_VAR(&var_t::set_chrom_id); };
			
			pos			= integer %{ HANDLE_INTEGER_END_VAR(&var_t::set_pos); };
			
			id_part		= (([.] | (chr - ';'))+)
				>(start_string)
				%{ HANDLE_STRING_END_VAR(&var_t::set_id, subfield_idx++); };
			id_rec		= (id_part (';' id_part)*) >{ subfield_idx = 0; };
			
			ref			= ([ACGTN]+)
				>(start_string)
				%{ HANDLE_STRING_END_VAR(&var_t::set_ref); };
			
			# FIXME: add breakends.
			simple_alt	= ([ACGTN]+);
			complex_alt	= ([*]+) %{ alt_is_complex = true; };	# Use only '*' since a following definition has simple_alt | complex_alt.
			
			# Structural variants.
			sv_alt_id_chr		= chr - [<>:];	# No angle brackets in SV identifiers.
			
			# Only set UNKNOWN when entering any state of sv_alt_t_unknown (instead of the final state exiting transitions).
			# Otherwise the type will be set first to e.g. CNV and immediately to UNKNOWN.
			sv_alt_t_del		= 'DEL'				% (sv_t, 2)	 %{ CURRENT_ALT.alt_sv_type = sv_type::DEL; };
			sv_alt_t_ins		= 'INS'				% (sv_t, 2)	 %{ CURRENT_ALT.alt_sv_type = sv_type::INS; };
			sv_alt_t_dup		= 'DUP'				% (sv_t, 2)	 %{ CURRENT_ALT.alt_sv_type = sv_type::DUP; };
			sv_alt_t_inv		= 'INV'				% (sv_t, 2)	 %{ CURRENT_ALT.alt_sv_type = sv_type::INV; };
			sv_alt_t_cnv		= 'CNV'				% (sv_t, 2)	 %{ CURRENT_ALT.alt_sv_type = sv_type::CNV; };
			sv_alt_t_dup_tandem	= 'DUP:TANDEM'		% (sv_t, 2)	 %{ CURRENT_ALT.alt_sv_type = sv_type::DUP_TANDEM; };
			sv_alt_t_del_me		= 'DEL:ME'			% (sv_t, 2)	 %{ CURRENT_ALT.alt_sv_type = sv_type::DEL_ME; };
			sv_alt_t_ins_me		= 'INS:ME'			% (sv_t, 2)	 %{ CURRENT_ALT.alt_sv_type = sv_type::INS_ME; };
			sv_alt_t_unknown	= sv_alt_id_chr+	% (sv_t, 1)	$~{ CURRENT_ALT.alt_sv_type = sv_type::UNKNOWN; };
			
			sv_alt_predef		= (
									sv_alt_t_del |
									sv_alt_t_ins |
									sv_alt_t_dup |
									sv_alt_t_inv |
									sv_alt_t_cnv |
									sv_alt_t_dup_tandem |
									sv_alt_t_del_me |
									sv_alt_t_ins_me |
									sv_alt_t_unknown
								);
			
			sv_alt_subtype		= sv_alt_id_chr+;
			sv_alt				= ('<' sv_alt_predef (':' sv_alt_subtype)* '>');
			
			alt_part	= (simple_alt | complex_alt | sv_alt)
				>(start_alt)
				%{
					// Complex ALTs not handled currently.
					if (alt_is_complex)
					{
						// FIXME: report an error.
						m_current_variant.m_alts.pop_back();
					}
					else
					{
						HANDLE_STRING_END_ALT(&alt_t::set_alt);
					}
				};
			alt				= (alt_part (',' alt_part)*) >{ subfield_idx = 0; };
			
			qual_numeric	= integer %{ HANDLE_INTEGER_END_VAR(&var_t::set_qual); };
			qual_unknown	= '.'
				$(set_unknown_quality_value)
				%{ HANDLE_INTEGER_END_VAR(&var_t::set_qual); };
			
			qual			= qual_numeric | qual_unknown;
			
			# FIXME: add actions.
			filter_pass	= 'PASS';
			filter_part	= (chr - ';')+;
			filter		= (filter_pass | (filter_part (';' filter_part)*));
			
			# Perform have_multiple_info_values before storing the current INFO value.
			info_str		= (chr - [,;=]) +;
			info_key		= (info_str)						>(start_string) %(end_info_key);
			info_val		= (info_str)						>(start_string) %(end_info_val);
			info_values		= (info_val (',' info_val)*)		>(start_info_values) %(end_info_values);
			info_assignment	=	(
									('=' info_values) >(info_not_flag) |
									"" >(info_flag)
								);
			info_part		= (info_key info_assignment)		>(start_info_kv) %(end_info_kv);
			info_missing	= '.';
			info			= (info_missing | (info_part (';' info_part)*)) >(start_info) %(end_info);
			
			format_part		= [^\t\n:]+;
			format			= (format_part (':' format_part)*) >(start_string) %(end_format);
			
			sep				= '\t';		# Field separator
			
			# Handle a newline and continue.
			# The newline gets eaten before its checked, though, so use any instead.
			main_nl			:= any @{ fhold; fgoto main; }	$eof{ retval = false; };
			break_nl		:= any @{ fhold; fbreak; }		$eof{ retval = false; };
			
			# #CHROM
			chrom_id_f :=
				(chrom_id sep)
				@{
					fgoto *check_max_field <fentry(main_nl), fentry(break_nl)>(vcf_field::POS, fentry(pos_f), cb);
				}
				$err(error);
			
			# POS
			pos_f :=
				(pos sep)
				@{
					fgoto *check_max_field <fentry(main_nl), fentry(break_nl)>(vcf_field::ID, fentry(id_f), cb);
				}
				$err(error);
			
			# ID
			id_f :=
				(id_rec sep)
				@{
					fgoto *check_max_field <fentry(main_nl), fentry(break_nl)>(vcf_field::REF, fentry(ref_f), cb);
				}
				$err(error);
			
			# REF
			ref_f :=
				(ref sep)
				@{
					fgoto *check_max_field <fentry(main_nl), fentry(break_nl)>(vcf_field::ALT, fentry(alt_f), cb);
				}
				$err(error);
			
			# ALT
			alt_f :=
				(alt sep)
				@{
					fgoto *check_max_field <fentry(main_nl), fentry(break_nl)>(vcf_field::QUAL, fentry(qual_f), cb);
				}
				$err(error);
			
			# QUAL
			qual_f :=
				(qual sep)
				@{
					fgoto *check_max_field <fentry(main_nl), fentry(break_nl)>(vcf_field::FILTER, fentry(filter_f), cb);
				}
				$err(error);
			
			# FILTER
			filter_f :=
				(filter sep)
				@{
					fgoto *check_max_field <fentry(main_nl), fentry(break_nl)>(vcf_field::INFO, fentry(info_f), cb);
				}
				$err(error);
			
			# INFO
			info_f :=
				(info sep)
				@{
					fgoto *check_max_field <fentry(main_nl), fentry(break_nl)>(vcf_field::FORMAT, fentry(format_f), cb);
				}
				$err(error);
			
			# FORMAT
			format_f :=
				(format sep)
				@{
					fgoto *check_max_field <fentry(main_nl), fentry(break_nl)>(vcf_field::ALL, fentry(sample_rec_f), cb);
				}
				$err(error);
			
			# Sample records
			sample_field	= ([^\t\n:]+) >(start_string) %(end_sample_field);
			sample_rec		= (sample_field (":" sample_field)*) >(start_sample) %(end_sample);
			sample_rec_f	:= (sample_rec ("\t" sample_rec)*)? %(end_last_sample) "\n" $err(error);
			
			# Line start.
			# Apparently main has to be able to read a character, so use fhold.
			# Allow EOF, though, with '?'.
			main := any?
				${
					fhold;
					
					subfield_idx = 0;
					sample_idx = 0;
					alt_is_complex = false;
					++m_lineno;
					m_current_variant.reset();
					m_current_variant.m_variant_index = m_variant_index++;
					m_current_variant.m_lineno = m_lineno;
					line_start = 1 + fpc;
					
					fgoto *check_max_field <fentry(main_nl), fentry(break_nl)>(vcf_field::CHROM, fentry(chrom_id_f), cb);
				}
				$err(error)
				$eof{ retval = false; };
			
			write init;
			write exec;
		}%%
		
		return retval;
	}
}

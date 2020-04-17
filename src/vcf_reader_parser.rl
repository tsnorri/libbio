/*
 * Copyright (c) 2017-2019 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <boost/format.hpp>
#include <boost/spirit/include/qi.hpp>
#include <libbio/assert.hh>
#include <libbio/vcf/subfield.hh>
#include <libbio/vcf/variant.hh>
#include <libbio/vcf/vcf_reader.hh>
#include "vcf_reader_private.hh"


%% machine vcf_parser;
%% write data;


namespace libbio::vcf {
	
	// Parse the fields until m_max_parsed field, find the next newline and invoke the callback function.
	template <int t_continue, int t_break, typename t_cb>
	int reader::check_max_field(field const vcf_field, int const target, bool const stop_after_newline, t_cb const &cb, bool &retval)
	{
		if (vcf_field <= m_max_parsed_field)
			return target;
		
		skip_to_next_nl();
		bool const st(cb(m_current_variant));
		++m_counter;
		
		if (!st)
		{
			retval = false;
			return t_break;
		}
		else if (stop_after_newline)
		{
			return t_break;
		}
		return t_continue;
	}
	
	
	template <typename t_cb>
	bool reader::parse2(t_cb const &cb, parser_state &state, bool const stop_after_newline)
	{
		typedef transient_variant				var_t;
		typedef variant_alt <std::string_view>	alt_t;
		
		bool retval(true);
		
		info_field_base			*current_info_field{};
		
		// State relative to the current line; may be discarded between calls to the parser.
		char const				*start(nullptr);			// Current string start.
		std::int64_t			integer(0);					// Currently read from the input.
		std::size_t				sample_idx(0);				// Current sample idx (1-based).
		std::size_t				subfield_idx(0);			// Current index in multi-part fields.
		int						cs(0);
		bool					integer_is_negative(false);	// Re-initialized in HANDLE_INTEGER_END.
		bool					gt_is_phased(false);		// Is the current GT phased.
		bool					alt_is_complex(false);		// Is the current ALT “complex” (includes *).
		bool					info_is_flag(false);
		
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
			
			action begin_qual {
				m_current_variant.set_qual(variant_base::UNKNOWN_QUALITY);
			}
			
			action end_qual_value {
				double qual{};
				std::string_view const qual_sv(start, fpc - start);
				if (boost::spirit::qi::parse(qual_sv.begin(), qual_sv.end(), boost::spirit::qi::float_, qual))
					m_current_variant.set_qual(qual);
				else
					throw std::runtime_error("Unable to parse the given value");
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

			action end_filter_name {
				{
					auto const &filters(m_current_variant.m_reader->metadata().filter());
					auto const filter_name(std::string_view(start, fpc - start));
					auto const it(filters.find(filter_name));
					if (filters.end() == it)
						throw std::runtime_error((boost::format("Unknown FILTER name ‘%s’") % filter_name).str());
					m_current_variant.m_filters.emplace_back(&it->second);
				}
			}
			
			action end_format {
				std::string_view const new_format(start, fpc - start);
				if (new_format != state.current_format)
				{
					m_current_format->reader_will_update_format(*this);
					m_current_variant.deinitialize_samples();
					
					state.current_format = new_format;
					parse_format(state.current_format);
					auto const [size, alignment] = assign_format_field_indices_and_offsets();
					auto const field_count(m_current_format->fields_by_identifier().size());
					m_current_variant.reserve_memory_for_samples(size, alignment, field_count);
					
					m_current_variant.initialize_samples();
					m_current_format->reader_did_update_format(*this);
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
				{
					auto &samples(m_current_variant.m_samples);
					libbio_always_assert_lt_msg(sample_idx, samples.size(), "Samples should be preallocated.");
					auto &current_sample(samples[sample_idx]);
					current_sample.reset();
					for (auto const *field_ptr : m_current_format_vec)
						field_ptr->prepare(current_sample);
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
				
				if (!cb(m_current_variant))
				{
					retval = false;
					fhold;
					fbreak;
				}
				
				++m_counter;
				
				if (stop_after_newline)
				{
					fnext main;
					fbreak;
				}
				
				fgoto main;
			}
			
			action error {
				report_unexpected_character(fpc, fpc - m_current_line_start, fcurs);
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
			simple_alt			= ([ACGTN]+);
			alt_allele_missing	= '*';
			
			# Structural variants.
			sv_alt_id_chr		= chr - [<>:];	# No angle brackets in SV identifiers.
			
			# Only set UNKNOWN when entering any state of sv_alt_t_unknown (instead of the final state exiting transitions).
			# Otherwise the type will be set first to e.g. CNV and then immediately to UNKNOWN.
			sv_alt_t_del		= 'DEL'				% (sv_t, 2)	 %{ CURRENT_ALT.alt_sv_type = sv_type::DEL; };
			sv_alt_t_ins		= 'INS'				% (sv_t, 2)	 %{ CURRENT_ALT.alt_sv_type = sv_type::INS; };
			sv_alt_t_dup		= 'DUP'				% (sv_t, 2)	 %{ CURRENT_ALT.alt_sv_type = sv_type::DUP; };
			sv_alt_t_inv		= 'INV'				% (sv_t, 2)	 %{ CURRENT_ALT.alt_sv_type = sv_type::INV; };
			sv_alt_t_cnv		= 'CNV'				% (sv_t, 2)	 %{ CURRENT_ALT.alt_sv_type = sv_type::CNV; };
			sv_alt_t_dup_tandem	= 'DUP:TANDEM'		% (sv_t, 2)	 %{ CURRENT_ALT.alt_sv_type = sv_type::DUP_TANDEM; };
			sv_alt_t_del_me		= 'DEL:ME'			% (sv_t, 2)	 %{ CURRENT_ALT.alt_sv_type = sv_type::DEL_ME; };
			sv_alt_t_ins_me		= 'INS:ME'			% (sv_t, 2)	 %{ CURRENT_ALT.alt_sv_type = sv_type::INS_ME; };
			sv_alt_t_unknown	= sv_alt_id_chr+	% (sv_t, 1)	$~{ CURRENT_ALT.alt_sv_type = sv_type::UNKNOWN_SV; };
			
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
			
			action end_alt_string { HANDLE_STRING_END_ALT(&alt_t::set_alt); }
			action end_alt_unknown {
				HANDLE_STRING_END_ALT(&alt_t::set_alt);
				CURRENT_ALT.alt_sv_type = sv_type::UNKNOWN;
			}
			alt_string	= (simple_alt | alt_allele_missing | sv_alt)
				>(start_alt)
				%(end_alt_string);
			alt_unknown		= '.' >(start_alt) %(end_alt_unknown);
			alt_part		= alt_string | alt_unknown;
			alt				= (alt_part (',' alt_part)*) >{ subfield_idx = 0; };
			
			qual_numeric	= (([0-9]+ ('.' [0-9]+)?) | ('.' [0-9]+)) >(start_string) %(end_qual_value);
			qual_unknown	= '.';
			
			qual			= (qual_numeric | qual_unknown) >begin_qual;
			
			# For now, values not equal to “PASS” are stored.
			# FIXME: currently “.” cannot be told apart from PASS.
			filter_pass		= 'PASS';
			filter_unknown	= '.';
			filter_name		= (chr - ';')+ - ('.' | 'PASS');
			filter_part		= filter_name >(start_string) %(end_filter_name);
			filter			= (filter_pass | filter_unknown | (filter_part (';' filter_part)*));
			
			# Perform have_multiple_info_values before storing the current INFO value.
			info_str		= (chr - [,;=])+;
			info_key		= ([A-Za-z_][0-9A-Za-z_.]*|"1000G")	>(start_string) %(end_info_key);
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
			
			sep				= [\t];		# Field separator
			
			# Handle a newline and continue.
			main_nl			:= '\n' >to{ fhold; }	@{ fgoto main; }	$eof{ throw std::runtime_error("Got an unexpected EOF"); };
			break_nl		:= '\n' >to{ fhold; }	@{ fbreak; }		$eof{ throw std::runtime_error("Got an unexpected EOF"); };
			
			# #CHROM
			chrom_id_f :=
				(chrom_id sep)
				@{
					fgoto *check_max_field <fentry(main_nl), fentry(break_nl)>(field::POS, fentry(pos_f), stop_after_newline, cb, retval);
				}
				$err(error);
			
			# POS
			pos_f :=
				(pos sep)
				@{
					fgoto *check_max_field <fentry(main_nl), fentry(break_nl)>(field::ID, fentry(id_f), stop_after_newline, cb, retval);
				}
				$err(error);
			
			# ID
			id_f :=
				(id_rec sep)
				@{
					fgoto *check_max_field <fentry(main_nl), fentry(break_nl)>(field::REF, fentry(ref_f), stop_after_newline, cb, retval);
				}
				$err(error);
			
			# REF
			ref_f :=
				(ref sep)
				@{
					fgoto *check_max_field <fentry(main_nl), fentry(break_nl)>(field::ALT, fentry(alt_f), stop_after_newline, cb, retval);
				}
				$err(error);
			
			# ALT
			alt_f :=
				(alt sep)
				@{
					fgoto *check_max_field <fentry(main_nl), fentry(break_nl)>(field::QUAL, fentry(qual_f), stop_after_newline, cb, retval);
				}
				$err(error);
			
			# QUAL
			qual_f :=
				(qual sep)
				@{
					fgoto *check_max_field <fentry(main_nl), fentry(break_nl)>(field::FILTER, fentry(filter_f), stop_after_newline, cb, retval);
				}
				$err(error);
			
			# FILTER
			filter_f :=
				(filter sep)
				@{
					fgoto *check_max_field <fentry(main_nl), fentry(break_nl)>(field::INFO, fentry(info_f), stop_after_newline, cb, retval);
				}
				$err(error);
			
			# INFO
			info_f :=
				(info (sep | [\n]))
				@{
					fgoto *check_max_field <fentry(main_nl), fentry(break_nl)>(field::FORMAT, fentry(format_f), stop_after_newline, cb, retval);
				}
				$err(error);
			
			# FORMAT
			format_f :=
				(format sep)
				@{
					fgoto *check_max_field <fentry(main_nl), fentry(break_nl)>(field::ALL, fentry(sample_rec_f), stop_after_newline, cb, retval);
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
					m_current_line_start = 1 + fpc;
					
					fgoto *check_max_field <fentry(main_nl), fentry(break_nl)>(field::CHROM, fentry(chrom_id_f), stop_after_newline, cb, retval);
				}
				$err(error)
				$eof{ retval = false; }; # Expected EOF. FIXME: should be true? Works anyway when using memory mapping.
			
			write init;
			write exec;
		}%%
		
		return retval;
	}
	
	
	bool reader::parse_nc(callback_fn const &callback)							{ parser_state state; return parse2(callback, state, false); }
	bool reader::parse_nc(callback_fn &&callback)								{ parser_state state; return parse2(callback, state, false); }
	bool reader::parse(callback_cq_fn const &callback)							{ parser_state state; return parse2(callback, state, false); }
	bool reader::parse(callback_cq_fn &&callback)								{ parser_state state; return parse2(callback, state, false); }
	bool reader::parse_one_nc(callback_fn const &callback, parser_state &state)	{ return parse2(callback, state, true); }
	bool reader::parse_one_nc(callback_fn &&callback, parser_state &state)		{ return parse2(callback, state, true); }
	bool reader::parse_one(callback_cq_fn const &callback, parser_state &state)	{ return parse2(callback, state, true); }
	bool reader::parse_one(callback_cq_fn &&callback, parser_state &state)		{ return parse2(callback, state, true); }
}

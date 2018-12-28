/*
 * Copyright (c) 2017-2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <libbio/vcf_reader.hh>


%% machine vcf_parser;
%% write data;


// We would like to post-process strings that have been read and then pass
// them to the variant object with a simple function call at the call site.
// Solution: Templates can have member function pointers as parameters but of a specific type.
#define HANDLE_STRING_END(FN, ...)	caller <decltype(FN)>::handle_string_end <FN>(*this, ##__VA_ARGS__)
#define HANDLE_INTEGER_END(FN, ...)	caller <decltype(FN)>::handle_integer_end <FN>(*this, ##__VA_ARGS__)


namespace libbio { namespace detail {

	template <typename t_enum>
	constexpr typename std::underlying_type <t_enum>::type to_underlying(t_enum e)
	{
		return static_cast <typename std::underlying_type <t_enum>::type>(e);
	}
	

	// Split a t_string into string_views.
	template <bool t_emplace_back, typename t_string, typename t_invalid_pos>
	std::size_t read_fields(
		t_string const &string,
		t_invalid_pos const invalid_pos,
		char const *string_start,
		char const *sep,
		std::size_t const count,
		std::vector <std::string_view> &res
	)
	{
		char const *string_ptr(string_start);
		
		if (t_emplace_back)
			res.clear();
		else
			res.resize(count);
		
		typename t_string::size_type sep_pos(0);
		std::size_t i(0);
		bool should_break(false);
		while (i < count)
		{
			sep_pos = string.find(sep, sep_pos);
			if (invalid_pos == sep_pos)
			{
				should_break = true;
				sep_pos = string.size();
			}
			
			if (t_emplace_back)
				res.emplace_back(string_ptr, sep_pos - (string_ptr - string_start));
			else
			{
				std::string_view sv(string_ptr, sep_pos - (string_ptr - string_start));
				res[i] = std::move(sv);
			}
			
			++i;
			++sep_pos;
			string_ptr = string_start + sep_pos;
			
			if (should_break)
				break;
		}
		return i;
	}
	
	
	// Split a string into string_views.
	template <bool t_emplace_back>
	std::size_t read_fields(
		std::string const &string,
		char const *sep,
		std::size_t const count,
		std::vector <std::string_view> &res
	)
	{
		return read_fields <t_emplace_back>(string, std::string::npos, string.c_str(), sep, count, res);
	}
	
	
	template <bool t_emplace_back>
	std::size_t read_fields(
		std::string_view const &sv,
		char const *sep,
		std::size_t const count,
		std::vector <std::string_view> &res
	)
	{
		return read_fields <t_emplace_back>(sv, std::string_view::npos, static_cast <char const *>(sv.data()), sep, count, res);
	}
}}


namespace libbio {
	
	template <typename t_fnt>
	struct vcf_reader::caller
	{
		template <t_fnt t_fn, typename ... t_args>
		static void handle_string_end(vcf_reader &r, t_args ... args)
		{
			std::string_view sv(r.m_start, r.m_fsm.p - r.m_start);
			(r.m_current_variant.*(t_fn))(sv, std::forward <t_args>(args)...);
		}
		
		template <t_fnt t_fn, typename ... t_args>
		static void handle_integer_end(vcf_reader &r, t_args ... args)
		{
			(r.m_current_variant.*(t_fn))(r.m_integer, std::forward <t_args>(args)...);
		}
	};
	
	
	template <int t_continue, int t_break>
	int vcf_reader::check_max_field(vcf_field const field, int const target, callback_fn const &cb)
	{
		if (field <= m_max_parsed_field)
			return target;
		
		skip_to_next_nl();
		if (!cb(m_current_variant))
			return t_break;
		
		++m_counter;
		return t_continue;
	}


	void vcf_reader::report_unexpected_character(char const *current_character, int const current_state)
	{
		std::cerr
		<< "Unexpected character '" << *current_character << "' at " << m_lineno << ':' << (current_character - m_line_start)
		<< ", state " << current_state << '.' << std::endl;

		auto const start(m_fsm.pe - m_fsm.p < 128 ? m_fsm.p : m_fsm.pe - 128);
		std::string_view buffer_end(start, m_fsm.pe - start);
		std::cerr
		<< "** Last 128 charcters from the buffer:" << std::endl
		<< buffer_end << std::endl;

		abort();
	}
	
	
	void vcf_reader::skip_to_next_nl()
	{
		std::string_view sv(m_fsm.p, m_fsm.pe - m_fsm.p);
		auto const pos(sv.find('\n'));
		libbio_always_assert_msg(std::string_view::npos != pos, "Unable to find the next newline");
		m_fsm.p += pos;
	}
	
	
	// Seek to the beginning of the records.
	void vcf_reader::reset()
	{
		libbio_assert(m_input);
		m_input->reset_to_first_variant_offset();
		m_lineno = m_input->last_header_lineno();
		m_fsm.eof = nullptr;
		m_counter = 0;
		m_variant_index = 0;
	}
	
	
	// Return the 1-based number of the given sample.
	std::size_t vcf_reader::sample_no(std::string const &sample_name) const
	{
		auto const it(m_sample_names.find(sample_name));
		if (it == m_sample_names.cend())
			return 0;
		return it->second;
	}
	
	
	void vcf_reader::read_header()
	{
		// For now, just skip lines that begin with "##".
		std::string_view line;
		while (m_input->getline(line))
		{
			++m_lineno;
			if ('\n' == line[0])
				continue;
			
			if (! ('#' == line[0] && '#' == line[1]))
				break;
			
			// FIXME: header handling goes here.
		}
		
		// Check for column headers.
		{
			auto const prefix(std::string("#CHROM"));
			auto const res(mismatch(line.cbegin(), line.cend(), prefix.cbegin(), prefix.cend()));
			if (prefix.cend() != res.second)
			{
				std::cerr << "Expected the next line to start with '#CHROM', instead got: '" << line << "'" << std::endl;
				exit(1);
			}
		}
		
		// Read sample names.
		std::vector <std::string_view> header_names;
		detail::read_fields <true>(line, "\t", -1, header_names);
		std::size_t i(1);
		for (auto it(header_names.cbegin() + detail::to_underlying(vcf_field::ALL)), end(header_names.cend()); it != end; ++it)
		{
			std::string str(*it);
			auto const res(m_sample_names.emplace(std::move(str), i));
			libbio_always_assert_msg(res.second, "Duplicate sample name");
			++i;
		}
		
		// stream now points to the first variant.
		m_input->store_first_variant_offset(1 + m_lineno);
		
		// Instantiate a variant.
		transient_variant var(sample_count());
		using std::swap;
		swap(m_current_variant, var);
	}
	
	
	void vcf_reader::fill_buffer()
	{
		libbio_assert(m_input);
		m_input->fill_buffer(*this);
	}
	
	
	bool vcf_reader::parse(callback_fn &&callback)
	{
		return parse(callback);
	}
	
	
	bool vcf_reader::parse(callback_fn const &cb)
	{
		typedef variant_tpl <std::string_view> vc;
		bool retval(true);
		int cs(0);
		// FIXME: add throwing EOF actions?
		%%{
			variable p		m_fsm.p;
			variable pe		m_fsm.pe;
			variable eof	m_fsm.eof;
			
			action start_string {
				m_start = fpc;
			}
			
			action start_alt {
				m_start = fpc;
				m_alt_sv = sv_type::NONE;
			}
			
			action start_integer {
				m_integer = 0;
			}
			
			action update_integer {
				// Shift and add the current number.
				m_integer *= 10;
				m_integer += fc - '0';
			}
			
			action set_unknown_quality_value {
				m_integer = std::numeric_limits <decltype(m_integer)>::max();
			}
			
			action end_sample_field {
				// Check that the current field is followed by another field, another sample or a new record.
				switch (fc)
				{
					case ':':
						fgoto sample_rec_f;
					
					case '\t':
					{
						libbio_always_assert_msg(m_format.size() == m_format_idx, "Not all fields present in the sample");
						m_format_idx = 0;
						fgoto sample_rec_f;
					}
					
					case '\n':
					{
						libbio_always_assert_msg(m_format.size() == m_format_idx, "Not all fields present in the sample");
						
						if (!cb(m_current_variant))
						{
							fhold;
							fbreak;
						}
						
						++m_counter;
						fgoto main;
					}
					
					default:
						report_unexpected_character(fpc, fcurs);
				}
			}
			
			action error {
				report_unexpected_character(fpc, fcurs);
			}
			
			tab			= '\t';
			
			id_string	= '<' alnum+ '>';
			
			chr			= graph;
			
			chrom_id	= (chr+)
				>(start_string)
				%{ HANDLE_STRING_END(&vc::set_chrom_id); };
			
			pos			= (digit+)
				>(start_integer)
				$(update_integer)
				%{ HANDLE_INTEGER_END(&vc::set_pos); };
			
			id_part		= (([.] | (chr - ';'))+)
				>(start_string)
				%{ HANDLE_STRING_END(&vc::set_id, m_idx++); };
			id_rec		= (id_part (';' id_part)*) >{ m_idx = 0; };
			
			ref			= ([ACGTN]+)
				>(start_string)
				%{ HANDLE_STRING_END(&vc::set_ref); };
			
			# FIXME: add breakends.
			simple_alt	= ([ACGTN]+);
			complex_alt	= ([*]+) %{ m_alt_is_complex = true; };	# Use only '*' since a following definition has simple_alt | complex_alt.
			
			# Structural variants.
			sv_alt_id_chr		= chr - [<>:];	# No angle brackets in SV identifiers.
			
			# Only set UNKNOWN when entering any state of sv_alt_t_unknown (instead of the final state exiting transitions).
			# Otherwise the type will be set first to e.g. CNV and immediately to UNKNOWN.
			sv_alt_t_del		= 'DEL'				% (sv_t, 2)	%{ m_alt_sv = sv_type::DEL; };
			sv_alt_t_ins		= 'INS'				% (sv_t, 2)	%{ m_alt_sv = sv_type::INS; };
			sv_alt_t_dup		= 'DUP'				% (sv_t, 2)	%{ m_alt_sv = sv_type::DUP; };
			sv_alt_t_inv		= 'INV'				% (sv_t, 2)	%{ m_alt_sv = sv_type::INV; };
			sv_alt_t_cnv		= 'CNV'				% (sv_t, 2)	%{ m_alt_sv = sv_type::CNV; };
			sv_alt_t_dup_tandem	= 'DUP:TANDEM'		% (sv_t, 2)	%{ m_alt_sv = sv_type::DUP_TANDEM; };
			sv_alt_t_del_me		= 'DEL:ME'			% (sv_t, 2)	%{ m_alt_sv = sv_type::DEL_ME; };
			sv_alt_t_ins_me		= 'INS:ME'			% (sv_t, 2)	%{ m_alt_sv = sv_type::INS_ME; };
			sv_alt_t_unknown	= sv_alt_id_chr+	% (sv_t, 1)	$~{ m_alt_sv = sv_type::UNKNOWN; };
			
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
					m_current_variant.set_alt_sv_type(m_alt_sv, m_idx);
					HANDLE_STRING_END(&vc::set_alt, m_idx++, m_alt_is_complex);
				};
			alt			= (alt_part (',' alt_part)*) >{ m_idx = 0; };
			
			qual_numeric	= (digit+)
				>(start_integer)
				$(update_integer)
				%{ HANDLE_INTEGER_END(&vc::set_qual); };
			qual_unknown	= '.'
				$(set_unknown_quality_value)
				%{ HANDLE_INTEGER_END(&vc::set_qual); };
			
			qual			= qual_numeric | qual_unknown;
			
			# FIXME: add actions.
			filter_pass	= 'PASS';
			filter_part	= (chr - ';')+;
			filter		= (filter_pass | (filter_part (';' filter_part)*));
			
			# FIXME: add actions.
			info_str		= (chr - [,;=]) +;
			info_key		= info_str;
			info_val		= info_str (',' info_str)*;
			info_part		= info_key ('=' info_val)?;
			info_missing	= '.';
			info			= info_missing | (info_part (';' info_part)*);
			
			# FIXME: handle other formats.
			format_gt	= ('GT') @{ m_format.emplace_back(format_field::GT); };
			format_dp	= ('DP') @{ m_format.emplace_back(format_field::DP); };
			format_gq	= ('GQ') @{ m_format.emplace_back(format_field::GQ); };
			format_ps	= ('PS') @{ m_format.emplace_back(format_field::PS); };
			format_pq	= ('PQ') @{ m_format.emplace_back(format_field::PQ); };
			format_mq	= ('MQ') @{ m_format.emplace_back(format_field::MQ); };
			format_part	= format_gt | format_dp | format_gq | format_ps | format_pq | format_mq;
			format		= ((format_part (':' format_part)*) >{ m_format.clear(); });
			
			# Genotype field in sample.
			sample_gt_null_allele	= '.'
				%{
					m_integer = NULL_ALLELE;
					HANDLE_INTEGER_END(&vc::set_gt, m_sample_idx, m_idx++, m_gt_is_phased);
				};
				
			sample_gt_allele_idx	= (digit+)
				>(start_integer)
				$(update_integer)
				%{
					HANDLE_INTEGER_END(&vc::set_gt, m_sample_idx, m_idx++, m_gt_is_phased);
					m_gt_is_phased = false;
				};
				
			sample_gt_part			= (sample_gt_null_allele | sample_gt_allele_idx);
			sample_gt_p				= sample_gt_part ('|' ${ m_gt_is_phased = true; } sample_gt_part)+;
			sample_gt_up			= sample_gt_part ('/' sample_gt_part)+;
			sample_gt				= (sample_gt_part | sample_gt_p | sample_gt_up)
				>{
					m_idx = 0;
				};
			
			# FIXME: add actions.
			sample_dp		= (digit+);
			sample_gq		= (digit+);
			sample_ps		= (digit+);
			sample_pq		= (digit+);
			sample_mq		= (digit+);
			
			sep				= '\t';		# Field separator
			ssep			= [\t\n:];	# Sample separator
				
			# Handle a newline and continue.
			# The newline gets eaten before its checked, though, so use any instead.
			main_nl		:= any @{ fhold; fgoto main; }	$eof{ retval = false; };
			break_nl	:= any @{ fhold; fbreak; }		$eof{ retval = false; };
			
			# Line start.
			# Apparently main has to be able to read a character, so use fhold.
			# Allow EOF, though, with '?'.
			main := any?
				${
					fhold;
					
					m_idx = 0;
					m_format_idx = 0;
					m_sample_idx = 0;
					m_alt_is_complex = false;
					m_alt_sv = sv_type::NONE;
					++m_lineno;
					m_current_variant.reset();
					m_current_variant.set_variant_index(m_variant_index++);
					m_current_variant.set_lineno(m_lineno);
					m_line_start = 1 + fpc;
					
					fgoto *check_max_field <fentry(main_nl), fentry(break_nl)>(vcf_field::CHROM, fentry(chrom_id_f), cb);
				}
				$err(error)
				$eof{ retval = false; };
			
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
			# FIXME: try to skip parsing the field and compare to the existing format?
			format_f :=
				(format sep)
				@{
					fgoto *check_max_field <fentry(main_nl), fentry(break_nl)>(vcf_field::ALL, fentry(sample_rec_f), cb);
				}
				$err(error);
			
			# Sample fields
			sample_gt_f := ((sample_gt) ssep @(end_sample_field)) $err(error);
			sample_dp_f := ((sample_dp) ssep @(end_sample_field)) $err(error);
			sample_gq_f := ((sample_gq) ssep @(end_sample_field)) $err(error);
			sample_ps_f := ((sample_ps) ssep @(end_sample_field)) $err(error);
			sample_pq_f := ((sample_pq) ssep @(end_sample_field)) $err(error);
			sample_mq_f := ((sample_mq) ssep @(end_sample_field)) $err(error);
			
			# Sample record
			sample_rec_f := "" >to{
				libbio_always_assert_msg(m_format_idx < m_format.size(), "Format does not match the sample");

				// Sample index 0 is reserved for the reference.
				++m_sample_idx;

				// Parse according to the format field.
				auto const idx(m_format_idx);
				++m_format_idx;
				auto const field(m_format[idx]);
				switch (field)
				{
					case format_field::GT:
						fgoto sample_gt_f;
						
					case format_field::DP:
						fgoto sample_dp_f;
						
					case format_field::GQ:
						fgoto sample_gq_f;
						
					case format_field::PS:
						fgoto sample_ps_f;
						
					case format_field::PQ:
						fgoto sample_pq_f;
						
					case format_field::MQ:
						fgoto sample_mq_f;
						
					default:
						libbio_fail("Unexpected format value");
				}
			};
			
			dummy := any
				@{
					// Dummy action,
					// make sure that all the machines are instantiated.
					if (false)
					{
						fgoto chrom_id_f;
						fgoto pos_f;
						fgoto id_f;
						fgoto ref_f;
						fgoto alt_f;
						fgoto qual_f;
						fgoto filter_f;
						fgoto info_f;
						fgoto format_f;
						fgoto sample_rec_f;
						
						fgoto sample_gt_f;
						fgoto sample_dp_f;
						fgoto sample_gq_f;
						fgoto sample_ps_f;
						fgoto sample_pq_f;
						fgoto sample_mq_f;
					}
				};
			
			write init;
			write exec;
		}%%
		
		return retval;
	}
}

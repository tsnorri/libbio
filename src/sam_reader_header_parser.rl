/*
 * Copyright (c) 2023 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <cstdint>
#include <libbio/sam/parse_error.hh>
#include <libbio/sam/reader.hh>
#include <stdexcept>
#include <string_view>


#define LAST_REF_SEQ	(header_.reference_sequences.back())
#define LAST_READ_GROUP	(header_.read_groups.back())
#define LAST_PROG		(header_.programs.back())
#define LAST_COMMENT	(header_.comments.back())


%% machine sam_header_parser;
%% write data;


namespace libbio::sam {
	
	void reader::read_header(header &header_, input_range_base &fsm) const
	{
		std::int64_t	integer{};				// Currently read from the input.
		bool			integer_is_negative{};
		bool			should_continue{true};
		
		char const		*eof{};
		int				cs{};
		
		%%{
			machine sam_header_parser;
			
			variable p		fsm.it;
			variable pe		fsm.sentinel;
			variable eof	eof;
			
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
			
			integer = ("+" | ("-" >{ integer_is_negative = true; }))? ([0-9]+) >(start_integer) $(update_integer) %(finish_integer);
			
			# We are currently quite flexible w.r.t. the character set requirements.
			identifier = [^\t\n]*;
			
			any_field = [A-Za-z]{2} ":";
			
			# @HD
			hd_version_major	= integer %{ header_.version_major = integer; };
			hd_version_minor	= integer %{ header_.version_minor = integer; };
			
			hd_sort_order		=	"unsorted"		@(hd_so, 2) %{ header_.sort_order = sort_order_type::unsorted; } |
									"queryname"		@(hd_so, 2) %{ header_.sort_order = sort_order_type::queryname; } |
									"coordinate"	@(hd_so, 2) %{ header_.sort_order = sort_order_type::coordinate; } |
									identifier		@(hd_so, 1);
			
			hd_grouping			=	"query"			@(hd_go, 2) %{ header_.grouping = grouping_type::query; } |
									"reference"		@(hd_go, 2) %{ header_.grouping = grouping_type::reference; } |
									identifier		@(hd_go, 1);
			
			hd_field	=	"VN:"		@(hd, 2) hd_version_major "." hd_version_minor	|
							"SO:"		@(hd, 2) hd_sort_order |
							"GO:"		@(hd, 2) hd_grouping |
							any_field	@(hd, 1) identifier;
			
			hd_entry	= "@HD" ("\t" hd_field)+;
			
			# @SQ
			action add_ref_seq {
				header_.reference_sequences.emplace_back();
			}
			
			sq_molecule_topology	=	"linear"	@(sq_tp, 2) %{ LAST_REF_SEQ.molecule_topology = molecule_topology_type::linear; } |
										"circular"	@(sq_tp, 2) %{ LAST_REF_SEQ.molecule_topology = molecule_topology_type::circular; } |
										identifier	@(sq_tp, 1);
				
			sq_field	=	"SN:"		@(sq, 2) identifier			${ LAST_REF_SEQ.name.push_back(fc); } |
							"LN:"		@(sq, 2) integer			%{ LAST_REF_SEQ.length = integer; } |
							"TP:"		@(sq, 2) sq_molecule_topology |
							any_field	@(sq, 1) identifier;
			
			sq_entry	= "@SQ" %(add_ref_seq) ("\t" sq_field)+;
			
			# @RG
			action add_read_group {
				header_.read_groups.emplace_back();
			}
			
			rg_field	=	"ID:"		@(rg, 2) identifier ${ LAST_READ_GROUP.id.push_back(fc); }
							"DS:"		@(rg, 2) identifier ${ LAST_READ_GROUP.description.push_back(fc); }
							any_field	@(rg, 1) identifier;
				
			rg_entry	= "@RG" %(add_read_group) ("\t" rg_field)+;
			
			# @PG
			action add_program {
				header_.programs.emplace_back();
			}
			
			pg_field	=	"ID:"		@(pg, 2) identifier ${ LAST_PROG.id.push_back(fc); }				|
							"PN:"		@(pg, 2) identifier ${ LAST_PROG.name.push_back(fc); }			|
							"CL:"		@(pg, 2) identifier ${ LAST_PROG.command_line.push_back(fc); }	|
							"PP:"		@(pg, 2) identifier ${ LAST_PROG.prev_id.push_back(fc); }		|
							"DS:"		@(pg, 2) identifier ${ LAST_PROG.description.push_back(fc); }	|
							"VN:"		@(pg, 2) identifier ${ LAST_PROG.version.push_back(fc); }		|
							any_field	@(pg, 1) identifier;
			
			pg_entry	= "@PG" %(add_program) ("\t" pg_field)+;
			
			# @CO
			comment_entry = "@CO\t" ([^\n]* >{ header_.comments.emplace_back(); } ${ LAST_COMMENT.push_back(fc); });
			
			# Main
			action unexpected_eof {
				throw parse_error("Unexpected EOF");
			}
			
			action error {
				throw parse_error("Unexpected character");
			}
			
			line = (hd_entry | sq_entry | rg_entry | pg_entry | comment_entry) $eof(unexpected_eof) "\n";
			non_header = [^@] >{ fhold; should_continue = false; };
			main := ((line*) $err(error)) non_header? %{ sort_reference_sequence_identifiers(header_); return; };
			
			write init;
		}%%
		
		header_.clear();
		while (true)
		{
			%% write exec;
			fsm.update();
		}
	}
}

/*
 * Copyright (c) 2023 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <libbio/sam/optional_field.hh>
#include <libbio/sam/parse_error.hh>
#include <libbio/sam/reader.hh>


#define CURRENT_STRING	(std::string_view{start, fsm.p + 1})


%% machine sam_optional_field_parser;
%% write data;


namespace libbio::sam {
	
	void reader::read_optional_fields(optional_field &dst, range_base &fsm) const
	{
		optional_field::tag_id_type	tag_id{};
		char const					*start{};
		std::int64_t				integer{};
		bool						integer_is_negative{};
		std::string					fp_buffer{};
		float						fp{};
		std::byte					byte{};
		
		char const					*eof{};
		int							cs{};
		
		%%{
			machine sam_optional_field_parser;
			
			variable p		fsm.it;
			variable pe		fsm.sentinel;
			variable eof	eof;
			
			action tag_id_first {
				tag_id = fc;
				tag_id <<= 8U;
			}
			
			action tag_id_second {
				tag_id |= fc;
			}
			
			action optional_start {
				tag_id = 0;
			}
			
			action handle_character {
				dst.add_value <char>(fc);
			}
			
			action integer_is_negative { integer_is_negative = true; }
			action start_integer { integer = 0; }
			action update_integer {
				// Shift and add the current number.
				integer *= 10;
				integer += fc - '0';
			}
			action finish_integer_ {
				if (integer_is_negative) integer *= -1;
				integer_is_negative = false;
			}
			action finish_integer {
				dst.add_value <std::int32_t>(integer);
			}
			
			action start_float { fp_buffer.clear(); }
			action update_float { fp_buffer.emplace_back(fc); }
			action finish_float_ {
				fp = 0;
				if (!boost::spirit::qi::parse(fp_buffer.begin(), fp_buffer.end(), boost::spirit::qi::float_, fp))
					throw parse_error("Unable to parse the given value as floating point", fp_buffer);;
			}
			action finish_float {
				dst.add_value <float>(tag_id, fp);
			}
			
			action start_string { start = fpc; }
			action finish_string {
				dst.add_value(tag_id, CURRENT_STRING);
			}
			
			action start_byte { byte = 0; }
			action byte_n { byte |= fpc - '0'; }
			action byte_a { byte |= fpc - 'A'; }
			action byte_1 { byte <<= 4; }
			action start_byte_array {
				dst.start_array <std::byte>(tag_id);
			}
			action update_byte_array {
				dst.add_array_value <std::byte>(byte);
			}
			
			sign				= '+' | '-';
			integer_			= ('+' | '-' %(integer_is_negative)) ([0-9]+) >(start_integer) $(update_integer) %(finish_integer_);
			float_				= (sign? [0-9]* [.]? [0-9]+ ([eE] sign? [0-9]+)?) >(start_float) $(update_float);
			integer				= integer_ %(finish_integer);
			float				= float_ %(finish_float);
			string				= [ !-~]* >(start_string) %(finish_string);
			byte				= ([0-9] %(byte_n) | [A-F] %(byte_a));
			byte_array			= ((byte %(byte_1) byte) >(start_byte) %(update_byte_array))* >(start_byte_array);
			# The way I read the specification, the parser is not needed to round fp values to integers if the array is declared to contain integers. Hence I should use fp syntex only for the fp array.
			integer_array_c		= [c] >{ dst.start_array <std::int8_t>(tag_id); }   ([,] integer_ %{ dst.add_array_value <std::int8_t>  (integer); })*;
			integer_array_C		= [C] >{ dst.start_array <std::uint8_t>(tag_id); }  ([,] integer_ %{ dst.add_array_value <std::uint8_t> (integer); })*;
			integer_array_s		= [s] >{ dst.start_array <std::int16_t>(tag_id); }  ([,] integer_ %{ dst.add_array_value <std::int16_t> (integer); })*;
			integer_array_S		= [S] >{ dst.start_array <std::uint16_t>(tag_id); } ([,] integer_ %{ dst.add_array_value <std::uint16_t>(integer); })*;
			integer_array_i		= [i] >{ dst.start_array <std::int32_t>(tag_id); }  ([,] integer_ %{ dst.add_array_value <std::int32_t> (integer); })*;
			integer_array_I		= [I] >{ dst.start_array <std::uint32_t>(tag_id); } ([,] integer_ %{ dst.add_array_value <std::uint32_t>(integer); })*;
			integer_array		= integer_array_c | integer_array_C | integer_array_s | integer_array_S | integer_array_i | integer_array_I;
			float_array			= [f] >{ dst.start_array <float>(tag_id); } ([,] float_ %{ dst.add_array_value <float>(fp); })*;
			
			value_character		= "A:" [!-~] %(handle_character);
			value_integer		= "i:" integer;
			value_float			= "f:" float;
			value_string		= "Z:" string;
			value_byte_array	= "H:" byte_array;
			value_numeric_array	= "B:" (integer_array | float_array);
			
			tag					= [A-Za-z] %(tag_id_first) [A-Za-z0-9] %(tag_id_second);
			typed_value			= value_character | value_integer | value_float | value_string | value_byte_array | value_numeric_array;
			optional_field		= (tag ':' typed_value) >(optional_start);
			
			action finish_loop { dst.update_tag_order(); return; }
			action error { throw parse_error("Unexpected character"); }
			action unexpected_eof { throw parse_error("Unexpected EOF"); }
			main := ((optional_field ('\t' optional_field*))? '\n') %(finish_loop) $err(error) @eof(unexpected_eof);
		}%%
		
		dst.clear();
		while (true)
		{
			%% write exec;
			fsm.update();
		}
	}
}
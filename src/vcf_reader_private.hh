/*
 * Copyright (c) 2017-2019 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_READER_PRIVATE_HH
#define LIBBIO_VCF_READER_PRIVATE_HH

#include <cstddef>
#include <cstdint>
#include <libbio/utility.hh>
#include <libbio/vcf/vcf_reader.hh>


// We would like to post-process strings that have been read and then pass
// them to the variant object with a simple function call at the call site.
// Solution: Templates can have member function pointers as parameters but of a specific type.
#define HANDLE_INTEGER(CALLS) do { CALLS; integer_is_negative = false; } while (false)

#define CURRENT_ALT								(m_current_variant.m_alts.back())
#define CURRENT_METADATA						(*current_metadata_ptr)
#define HANDLE_STRING_END_VAR(FN, ...)			caller <decltype(FN)>::handle_string_end	<FN>(start, m_fsm.p - start, m_current_variant, ##__VA_ARGS__)
#define HANDLE_STRING_END_ALT(FN, ...)			caller <decltype(FN)>::handle_string_end	<FN>(start, m_fsm.p - start, CURRENT_ALT, ##__VA_ARGS__)
#define HANDLE_STRING_END_METADATA(FN, ...)		caller <decltype(FN)>::handle_string_end	<FN>(start, m_fsm.p - start, CURRENT_METADATA, ##__VA_ARGS__)
#define HANDLE_INTEGER_END_VAR(FN, ...)			HANDLE_INTEGER(caller <decltype(FN)>::handle_integer_end	<FN>(integer, integer_is_negative, m_current_variant, ##__VA_ARGS__))
#define HANDLE_INTEGER_END_ALT(FN, ...)			HANDLE_INTEGER(caller <decltype(FN)>::handle_integer_end	<FN>(integer, integer_is_negative, CURRENT_ALT, ##__VA_ARGS__))
#define HANDLE_INTEGER_END_METADATA(FN, ...)	HANDLE_INTEGER(caller <decltype(FN)>::handle_integer_end	<FN>(integer, integer_is_negative, CURRENT_METADATA, ##__VA_ARGS__))


namespace libbio::vcf {
	
	template <typename t_fnt>
	struct reader::caller
	{
		template <t_fnt t_fn, typename t_dst, typename ... t_args>
		static void handle_string_end(char const *start, std::size_t length, t_dst &dst, t_args ... args)
		{
			std::string_view sv(start, length);
			(dst.*(t_fn))(sv, std::forward <t_args>(args)...);
		}
		
		template <t_fnt t_fn, typename t_dst, typename ... t_args>
		static void handle_integer_end(std::int64_t integer, bool const integer_is_negative, t_dst &dst, t_args ... args)
		{
			if (integer_is_negative)
				integer *= -1;
			(dst.*(t_fn))(integer, std::forward <t_args>(args)...);
		}
	};
}

#endif

/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_CXXCOMPAT_HH
#define LIBBIO_CXXCOMPAT_HH

#if __has_include(<string_view>)
#	include <string_view>
#elif __has_include(<experimental/string_view>)
#	include <experimental/string_view>

// XXX hack.
namespace std {
	using experimental::string_view;
}
#endif


#if __cplusplus < 202000L
// XXX hack.
#include <gsl/span>
namespace std {
	using gsl::span;
	
	template <class t_element, std::ptrdiff_t t_extent>
	span <gsl::byte const, gsl::details::calculate_byte_size <t_element, t_extent>::value> as_bytes(span <t_element, t_extent> s) noexcept { return gsl::as_bytes(s); }
}
#endif


#endif

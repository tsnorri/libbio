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

// FIXME hack.
namespace std {
	using experimental::string_view;
}
#endif

#endif

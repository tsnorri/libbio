/*
 * Copyright (c) 2023-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_SAM_TAG_DEFINITIONS_HH
#define LIBBIO_SAM_TAG_DEFINITIONS_HH

#include <cstdint>
#include <libbio/sam/literals.hh>
#include <libbio/sam/tag.hh>
#include <string>


namespace libbio::sam {

	template <> struct tag_value <"AS"_tag> { typedef std::int32_t	type; };
	template <> struct tag_value <"NM"_tag> { typedef std::int32_t	type; };
	template <> struct tag_value <"OA"_tag> { typedef std::string	type; };
}

#endif

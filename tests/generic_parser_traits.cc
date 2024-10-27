/*
 * Copyright (c) 2023-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <libbio/generic_parser.hh>
#include <type_traits>

namespace lbp	= libbio::parsing;


namespace {
	// Optional, repeating etc.
	struct optional { constexpr static bool const is_optional{true}; };
	struct optional_repeating { constexpr static bool const is_optional_repeating{true}; };
	struct repeating { constexpr static bool const is_repeating{true}; };

	struct not_optional { constexpr static bool const is_optional{false}; };
	struct not_optional_repeating { constexpr static bool const is_optional_repeating{false}; };
	struct not_repeating { constexpr static bool const is_repeating{false}; };

	struct not_specified {};

	static_assert(lbp::is_optional_v <optional>);
	static_assert(lbp::is_optional_v <optional_repeating>);
	static_assert(!lbp::is_optional_v <repeating>);
	static_assert(!lbp::is_optional_v <not_optional>);
	static_assert(!lbp::is_optional_v <not_optional_repeating>);
	static_assert(!lbp::is_optional_v <not_repeating>);
	static_assert(!lbp::is_optional_v <not_specified>);

	static_assert(!lbp::is_repeating_v <optional>);
	static_assert(lbp::is_repeating_v <optional_repeating>);
	static_assert(lbp::is_repeating_v <repeating>);
	static_assert(!lbp::is_repeating_v <not_optional>);
	static_assert(!lbp::is_repeating_v <not_optional_repeating>);
	static_assert(!lbp::is_repeating_v <not_repeating>);
	static_assert(!lbp::is_repeating_v <not_specified>);

	static_assert(!lbp::is_optional_repeating_v <optional>);
	static_assert(lbp::is_optional_repeating_v <optional_repeating>);
	static_assert(!lbp::is_optional_repeating_v <repeating>);
	static_assert(!lbp::is_optional_repeating_v <not_optional>);
	static_assert(!lbp::is_optional_repeating_v <not_optional_repeating>);
	static_assert(!lbp::is_optional_repeating_v <not_repeating>);
	static_assert(!lbp::is_optional_repeating_v <not_specified>);

	// Delimiters
	static_assert(std::is_same_v <lbp::delimiter <'\t', '\n'>, lbp::join_delimiters_t <lbp::delimiter <'\t'>, lbp::delimiter <'\n'>>>);

	typedef lbp::traits::delimited <lbp::delimiter <'\t'>, lbp::delimiter <'\n'>>::trait <5> trait_type;

	typedef trait_type::delimiter <not_specified, not_specified, 0> delimiter_for_initial;
	typedef trait_type::delimiter <not_specified, not_specified, 3> delimiter_for_middle;
	typedef trait_type::delimiter <not_specified, not_specified, 4> delimiter_for_final;
	typedef trait_type::delimiter <not_specified, optional, 3> delimiter_for_middle_with_next_optional;
	static_assert(std::is_same_v <lbp::delimiter <'\t'>, delimiter_for_initial>);
	static_assert(std::is_same_v <lbp::delimiter <'\t'>, delimiter_for_middle>);
	static_assert(std::is_same_v <lbp::delimiter <'\n'>, delimiter_for_final>);
	static_assert(std::is_same_v <lbp::delimiter <'\t', '\n'>, delimiter_for_middle_with_next_optional>);

	// Position.
	constexpr auto const initial{trait_type::template field_position <0, not_optional>};
	constexpr auto const middle{trait_type::template field_position <3, not_optional>};
	constexpr auto const middle_and_final{trait_type::template field_position <3, optional>};
	constexpr auto const final_{trait_type::template field_position <4, void>};
	static_assert(lbp::field_position::initial_ == initial);
	static_assert(lbp::field_position::middle_ == middle);
	static_assert((lbp::field_position::middle_ | lbp::field_position::final_) == middle_and_final);
	static_assert(lbp::field_position::final_ == final_);
}

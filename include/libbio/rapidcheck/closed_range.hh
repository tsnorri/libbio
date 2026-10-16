/*
 * Copyright (c) 2022-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_RAPIDCHECK_CLOSED_RANGE_HH
#define LIBBIO_RAPIDCHECK_CLOSED_RANGE_HH

#include <cassert>
#include <concepts>
#include <limits>
#include <rapidcheck.h>

// gen::inRange takes a half-open range but we would like to be able to generate e.g. UINT64_MAX.
namespace rc::gen {

	template <typename t_type>
	Gen <t_type> inClosedRange(t_type const min, t_type const max)
	{
		return [=](Random const &random, int size){
			if (max < min)
				throw GenerationFailure("Invalid range");

			auto const rangeSize(detail::scaleInteger(static_cast <Random::Number>(max) - static_cast <Random::Number>(min), size) + 1);
			auto const value(static_cast <t_type>((Random(random).next() % rangeSize) + min));
			assert(min <= value && value <= max);
			return shrinkable::shrinkRecur(value, [=](t_type const x){ return shrink::towards <t_type>(x, min); });
		};
	}


	template <std::floating_point t_type>
	Gen <t_type> inClosedRange(t_type const min, t_type const max)
	{
		return [=](Random const &random, int size){
			if (max < min)
				throw GenerationFailure("Invalid range");

			auto const rangeSize{max - min};

			// Scaling factor
			auto const numerator{detail::scaleInteger(Random(random).next(), size)};
			long double const denominator(std::numeric_limits<Random::Number>::max());
			auto const scale{numerator / denominator};

			t_type const value(min + scale * rangeSize);
			assert(min <= value && value <= max);

			return shrinkable::shrinkRecur(value, [=](t_type const x){ return shrink::towards <t_type>(x, min); });
		};
	}
}

#endif

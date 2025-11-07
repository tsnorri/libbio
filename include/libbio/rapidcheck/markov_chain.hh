/*
 * Copyright (c) 2022-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_RAPIDCHECK_MARKOV_CHAIN_HH
#define LIBBIO_RAPIDCHECK_MARKOV_CHAIN_HH

#include <cstdint>
#include <memory>					// std::make_unique
#include <libbio/markov_chain.hh>
#include <rapidcheck.h>
#include <utility>					// std::move
#include <vector>


namespace rc {

	template <typename t_base, typename t_initial_state, typename t_transitions>
	struct Arbitrary <libbio::markov_chains::chain <t_base, t_initial_state, t_transitions>>
	{
		typedef libbio::markov_chains::chain <t_base, t_initial_state, t_transitions>	chain_type;

		static Gen <chain_type> arbitrary()
		{
			// It seems to be quite difficult to functionally map a collection (of generators to a generator
			// that produces a collection), so we’ll resort to gen::exec. (We could use gen::mapCat to produce the
			// vector of probabilities but then using gen::arbitrary with the nodes / states would not be possible.)
			return gen::withSize([](int const size){
				return gen::exec([size](){
					auto const probabilities(
						*gen::container <std::vector <double>>(
							size,
							gen::map(
								// We need to use the half-open range here b.c. std::upper_bound is applied to find
								// the correct transition.
								gen::inRange <std::uint32_t>(0.0, UINT32_MAX),
								[](auto const val){
									return 1.0 * val / UINT32_MAX;
								}
							)
						)
					);

					chain_type retval;
					retval.values.reserve(1 + size);
					chain_type::visit_node_types(probabilities, [&retval]<typename t_type> {
						if constexpr (chain_type::uses_runtime_polymorphism)
						{
							// It seems that there is no other way to combine gen::arbitrary and operator new.
							retval.values.emplace_back(
								std::make_unique <t_type>(
									*gen::arbitrary <t_type>()
								)
							);
						}
						else
						{
							retval.values.emplace_back(*gen::arbitrary <t_type>()); // Needs a converting constructor.
						}

						return true;
					});
					return retval;
				});
			});
		}
	};
}

#endif

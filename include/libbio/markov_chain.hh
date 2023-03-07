/*
 * Copyright (c) 2022-2023 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_MARKOV_CHAIN_HH
#define LIBBIO_MARKOV_CHAIN_HH

#include <algorithm>				// std::sort, std::adjacent_find
#include <array>
#include <libbio/assert.hh>
#include <libbio/fmap.hh>			//libbio::map_to_array
#include <libbio/tuple/group_by.hh>
#include <libbio/tuple/filter.hh>
#include <libbio/tuple/for.hh>
#include <libbio/tuple/unique.hh>
#include <limits>					// std::numeric_limits
#include <memory>					// std::unique_ptr
#include <type_traits>				// std::is_same, std::bool_constant, std::is_base_of_v
#include <utility>					// std::pair, std::make_index_sequence
#include <vector>


namespace libbio::markov_chains::detail {
	
	template <typename, typename> struct callback_table_builder {};
	
	template <typename t_callback, typename... t_args>
	struct callback_table_builder <t_callback, std::tuple <t_args...>>
	{
		constexpr static std::array const fns{(&t_callback::template operator() <t_args>)...};
	};
	
	
	template <typename... t_args>
	struct aggregate : public t_args... {};
}


// FIXME: rewrite this s.t. defining the type still produces (ultimately) a constinit or constexpr Markov chain but the chain may also be built at run time.
namespace libbio::markov_chains {
	
	template <typename t_src, typename t_dst, double t_probability>
	struct transition
	{
		typedef t_src					source_type;
		typedef t_dst					destination_type;
		constexpr static double const	probability{t_probability};
	};
	
	
	template <typename... t_transitions>
	struct transition_list
	{
		typedef std::tuple <t_transitions...>	transitions_type;
	};
	
	
	template <typename t_initial_state, typename... t_others>
	struct transition_to_any_other
	{
		typedef std::tuple <t_others...>		non_initial_states_type;
		constexpr static inline auto const other_count{sizeof...(t_others)};
		constexpr static inline auto const other_count_1{other_count - 1};
		
		template <typename t_type>
		using from_initial_t = transition <t_initial_state, t_type, 1.0 / other_count>;
		
		template <typename t_src, typename t_dst>
		using from_other_t = transition <t_src, t_dst, 1.0 / other_count_1>;
		
		typedef tuples::map_t <non_initial_states_type, from_initial_t>	initial_transitions_type;
		
		// Determine the cross product, then remove pairs of the same type.
		// Finally make the transitions.
		typedef tuples::map <
			tuples::filter <
				tuples::cross_product_t <non_initial_states_type, non_initial_states_type, std::tuple>,
				tuples::negation <std::is_same>::with
			>,
			from_other_t
		>																non_initial_transitions_type;
		
		typedef tuples::cat_t <
			initial_transitions_type,
			non_initial_transitions_type
		>																transitions_type;
	};
	
	
	struct uses_runtime_polymorphism_trait {};
}


namespace libbio::markov_chains::detail {
	
	typedef std::size_t	node_type;
	
	template <typename t_transition> using transition_src_t = typename t_transition::source_type;
	template <typename t_transition> using transition_dst_t = typename t_transition::destination_type;
	
	template <typename, typename>
	struct transition_states_have_base {};
	
	template <typename t_base, typename... t_transition>
	struct transition_states_have_base <t_base, std::tuple <t_transition...>> :
		public std::bool_constant <
			(std::is_base_of_v <t_base, typename t_transition::source_type> && ...) &&
			(std::is_base_of_v <t_base, typename t_transition::destination_type> && ...)
		> {};
	
	template <typename t_base, typename t_tuple>
	constexpr inline bool const transition_states_have_base_v = transition_states_have_base <t_base, t_tuple>::value;
	
	
	// We would like to calculate the cumulative sum of probabilities for each equivalence class.
	// transitions_by_source_type has tuples that represent equivalence classes like
	// {src, {{src, dst, probability}, …}}, so we need to process them.
	template <typename t_transition, double t_cs>
	using update_transition_probability_t = transition <
		typename t_transition::source_type,
		typename t_transition::destination_type,
		t_transition::probability + t_cs
	>;
	
	template <typename t_transitions_ = tuples::empty, double t_cs = 0.0>
	struct transition_probability_cs_acc
	{
		typedef t_transitions_ transitions;
		constexpr static inline double const cs{t_cs};
	};
	
	template <
		typename t_acc,
		typename t_transition,
		typename t_updated_transition = update_transition_probability_t <t_transition, t_acc::cs>
	>
	using transition_probability_cs_fold_fn = transition_probability_cs_acc <
		tuples::append_t <typename t_acc::transitions, t_updated_transition>,	// Append to the transition list.
		t_updated_transition::probability										// Store the new probability.
	>;
	
	// Check that the sum of transition probabilities is 1.0.
	template <typename t_transitions>
	struct transition_probability_cs_check
	{
		// FIXME: compare with some epsilon since we calculate a sum of floating point values?
		static_assert(1.0 == tuples::last_t <t_transitions>::probability);
		typedef t_transitions transitions_type;
	};
	
	// Map each equivalence class of transitions s.t. the probabilities are changed to their cumulative sum.
	// The equivalence class representatives are discarded.
	template <
		typename t_eq_class,
		typename t_transitions = typename tuples::foldl_t <
			transition_probability_cs_fold_fn,
			transition_probability_cs_acc <>,
			tuples::second_t <t_eq_class>
		>::transitions,
		typename t_check = transition_probability_cs_check <t_transitions>
	>
	using transition_probability_cs_map_fn = typename t_check::transitions_type;
	
	struct transition_key
	{
		double		probability_threshold{};
		node_type	node{};
		
		constexpr auto as_tuple() const { return std::make_tuple(node, probability_threshold); }
		constexpr bool operator<(transition_key const &other) const { return as_tuple() < other.as_tuple(); }
		constexpr bool operator==(transition_key const &other) const { return as_tuple() == other.as_tuple(); }
	};
	
	template <typename t_nodes, typename t_transitions>
	struct transition_map_builder
	{
		typedef t_nodes										nodes_type;
		typedef std::pair <transition_key, node_type>		map_value_type;
		
		// Intermediate type for creating map_value_type.
		template <std::size_t t_src, std::size_t t_dst, double t_probability>
		struct transition_
		{
			constexpr map_value_type to_map_value() const { return {{t_probability, t_src}, t_dst}; }
			constexpr /* implicit */ operator map_value_type() const { return to_map_value(); }
		};
	
		// FIXME: this is really inefficient. Come up with another solution, e.g. store the indices in sorted vectors.
		template <typename t_transition>
		using to_transition__t = transition_ <
			tuples::first_index_of_v <nodes_type, typename t_transition::source_type>,
			tuples::first_index_of_v <nodes_type, typename t_transition::destination_type>,
			t_transition::probability
		>;
	
		constexpr static auto make_transition_map()
		{
			// Group the transitions by the source node.
			typedef typename tuples::group_by_type <
				t_transitions,
				transition_src_t
			>::keyed_type										transitions_by_source_type;
		
			// Build the transition table.
			typedef tuples::cat_with_t <
				tuples::map_t <
					transitions_by_source_type,
					detail::transition_probability_cs_map_fn
				>
			>													transitions_with_probability_cs_type;
		
			typedef tuples::map_t <
				transitions_with_probability_cs_type,
				to_transition__t
			>													intermediate_transitions_type;
		
			constexpr intermediate_transitions_type intermediates{};
			auto retval(libbio::map_to_array(
				std::make_index_sequence <std::tuple_size_v <t_transitions>>{},
				[&intermediates](auto const idx) {
					return std::get <idx()>(intermediates).to_map_value();
				}
			));
		
			std::sort(retval.begin(), retval.end());
		
			// Make sure that the keys are distinct.
			libbio_assert_eq(retval.cend(), std::adjacent_find(retval.cbegin(), retval.cend(), [](auto const &lhs, auto const &rhs){
				return lhs.first == rhs.first;
			}));
		
			return retval;
		}
	};
}


namespace libbio::markov_chains {
	
	// Since there is potentially a very large number of paths through the corresponding directed (cyclic) graph,
	// we can make use of runtime polymorphism if needed. The user can specify the base class of the instantiated
	// objects, though. This should result in an aggregate type as of C++20. (In particular, all the non-static
	// data members are public.)
	template <typename t_base, typename t_initial_state, typename t_transitions, typename... t_traits>
	struct chain : public t_traits...
	{
	private:
		typedef detail::aggregate <t_traits...>				traits_type;
		
	public:
		typedef t_initial_state								initial_state_type;
		typedef typename t_transitions::transitions_type	transitions_type;
		typedef detail::node_type							node_type;
		
		constexpr static auto const NODE_MAX{std::numeric_limits <node_type>::max()}; // Max. value of node_type.
		
		// Number the nodes.
		// This can be done easily by determining the unique types over the concatenation of the lists
		// of source and destination node types.
		typedef tuples::unique_t <
			tuples::cat_t <
				std::tuple <t_initial_state>,
				tuples::map_t <transitions_type, detail::transition_src_t>,
				tuples::map_t <transitions_type, detail::transition_dst_t>
			>
		>													nodes_type;
		
		constexpr static std::size_t const					node_limit{std::tuple_size_v <nodes_type>}; // The number of nodes in this chain.
		constexpr static bool const							uses_runtime_polymorphism{std::is_base_of_v <uses_runtime_polymorphism_trait, traits_type>};
		static_assert(!uses_runtime_polymorphism || std::is_base_of_v <t_base, t_initial_state>);
		static_assert(!uses_runtime_polymorphism || detail::transition_states_have_base_v <t_base, transitions_type>);
		
		typedef detail::transition_map_builder <
			nodes_type,
			transitions_type
		>													transition_map_builder_type;
		typedef transition_map_builder_type::map_value_type	transition_map_value_type;
		
	private:
		// The actual chain.
		constexpr static auto const			initial_state{tuples::first_index_of_v <nodes_type, t_initial_state>};
		constexpr static auto const			transitions{transition_map_builder_type::make_transition_map()};
		
	public:
		typedef std::vector <
			std::conditional_t <
				uses_runtime_polymorphism,
				std::unique_ptr <t_base>,
				t_base
			>
		>									values_type;
		
		values_type							values{};
		
		template <typename t_probabilities, typename t_visitor>
		static void visit_node_types(t_probabilities &&probabilities, t_visitor &&visitor)
		{
			typedef detail::transition_key transition_key;
			
			auto const &fns(detail::callback_table_builder <t_visitor, nodes_type>::fns);
			node_type current_node{initial_state};
			
			libbio_assert_lt(current_node, fns.size());
			(visitor.*(fns[current_node]))();
			for (auto const &prob : probabilities)
			{
				transition_key const key{prob, current_node};
				auto const it(std::upper_bound(
					transitions.begin(),
					transitions.end(),
					key,
					[](transition_key const &key, transition_map_value_type const &val){
						return key < val.first;
					}
				));
				libbio_assert_neq(transitions.cend(), it);
				current_node = it->second;
				libbio_assert_lt(current_node, fns.size());
				(visitor.*(fns[current_node]))();
			}
		}
		
		// No user-declared or inherited constructors allowed, so we construct with this static function template.
		template <typename t_probabilities>
		static chain from_probabilities(t_probabilities &&probabilities)
		requires uses_runtime_polymorphism
		{
			chain retval;
			retval.values.reserve(1 + probabilities.size());
			visit_node_types(probabilities, [&retval]<typename t_type> {
				if constexpr (uses_runtime_polymorphism)
					retval.values.emplace_back(std::make_unique <t_type>());
				else
					retval.values.emplace_back(t_type{}); // Needs a converting constructor.
			});
			return retval;
		}
	};
}

#endif

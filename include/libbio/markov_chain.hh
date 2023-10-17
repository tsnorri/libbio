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
#include <libbio/utility.hh>		// libbio::aggregate
#include <limits>					// std::numeric_limits
#include <memory>					// std::unique_ptr
#include <type_traits>				// std::is_same, std::bool_constant, std::is_base_of_v
#include <utility>					// std::pair, std::make_index_sequence
#include <vector>


namespace libbio::markov_chains::detail {
	
	constexpr inline bool compare_fp(double const lhs, double const rhs, double const epsilon)
	{
		// C++23’s constexpr std::fabs might not be available.
		return -epsilon < (lhs - rhs) && (lhs - rhs) < epsilon;
	}
	
	
	template <typename, typename> struct callback_table_builder {};
	
	template <typename t_callback, typename... t_args>
	struct callback_table_builder <t_callback, std::tuple <t_args...>>
	{
		constexpr static std::array const fns{(&t_callback::template operator() <t_args>)...};
	};
	
	
	// Clang (as of version 16) does not have non-type template arguments of type double, so we have to work around.
	struct wrapped_double
	{
		double value{};
		
		/* implicit */ constexpr wrapped_double(double const value_): value(value_) {}
		/* implicit */ constexpr operator double() const { return value; }
	};
	
	
	// Using tuples::cross_product_t was quite slow, so here is a faster implementation of filtering for transitions_between_any.
	template <std::size_t t_idx>
	struct index {};
	
	template <std::size_t... t_idxs>
	struct index_sequence {};
	
	template <typename> struct make_index_sequence_from_std {};
	
	template <std::size_t... t_idxs>
	struct make_index_sequence_from_std <std::index_sequence <t_idxs...>>
	{
		typedef index_sequence <t_idxs...> type;
	};
	
	template <typename t_type>
	using make_index_sequence_from_std_t = make_index_sequence_from_std <t_type>::type;
	
	template <std::size_t... t_lhs, std::size_t... t_rhs>
	constexpr index_sequence <t_lhs..., t_rhs...> operator+(index_sequence <t_lhs...>, index_sequence <t_rhs...>) { return {}; }
	
	template <std::size_t t_idx, typename t_filter>
	constexpr auto filter_one(index <t_idx>, t_filter predicate)
	{
		if constexpr (predicate(t_idx))
			return index_sequence <t_idx>{};
		else
			return index_sequence{};
	}
	
	template <std::size_t... t_idxs, typename t_filter>
	constexpr auto filter(index_sequence <t_idxs...>, t_filter predicate)
	{
		return (filter_one(index <t_idxs>{}, predicate) + ...);
	}
}


// FIXME: rewrite this s.t. defining the type still produces (ultimately) a constinit or constexpr Markov chain but the chain may also be built at run time.
namespace libbio::markov_chains {
	
	template <typename t_src, typename t_dst, detail::wrapped_double t_probability>
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
	
	
	template <typename... t_lists>
	using join_transition_lists = tuples::forward_t <tuples::cat_t <typename t_lists::transitions_type...>, transition_list>;
	
	
	template <typename t_initial_state, detail::wrapped_double t_total_probability, typename... t_others>
	struct transitions_to_any
	{
		typedef std::tuple <t_others...>	non_initial_states_type;
		constexpr static inline auto const other_count{sizeof...(t_others)};
		
		template <typename t_state>
		using from_initial_t = transition <t_initial_state, t_state, t_total_probability / other_count>;
		
		typedef tuples::map_t <non_initial_states_type, from_initial_t>	transitions_type;
	};
	
	
	template <typename t_target_state, detail::wrapped_double t_probability, typename... t_states>
	struct transitions_from_any
	{
		template <typename t_state>
		using from_t = transition <t_state, t_target_state, t_probability>;
		
		typedef tuples::map_t <std::tuple <t_states...>, from_t>	transitions_type;
	};
	
	
	template <detail::wrapped_double t_total_probability, typename... t_transitions>
	struct transitions_between_any
	{
		typedef std::tuple <t_transitions...>	given_transitions_type;
		constexpr static inline auto const transition_count{sizeof...(t_transitions)};
		constexpr static inline auto const transition_count_1{transition_count - 1};
		
		template <typename t_src, typename t_dst>
		using transition_t = transition <t_src, t_dst, t_total_probability / transition_count_1>;
		
		template <std::size_t t_idx>
		struct transition_at_index
		{
			typedef std::tuple_element_t <t_idx / transition_count, given_transitions_type> lhs_type;
			typedef std::tuple_element_t <t_idx % transition_count, given_transitions_type> rhs_type;
			typedef transition_t <lhs_type, rhs_type> type;
		};
		
		template <typename t_type> struct tuple_type {};
		
		template <std::size_t... t_idxs>
		struct tuple_type <detail::index_sequence <t_idxs...>>
		{
			typedef std::tuple <typename transition_at_index <t_idxs>::type...> type;
		};
		
		// Determine the cross product, then remove pairs of the same type as indicated by the index.
		constexpr static auto const indices{detail::filter(
			detail::make_index_sequence_from_std_t <std::make_index_sequence <transition_count * transition_count>>{},
			[](std::size_t const idx) constexpr {
				return idx / transition_count != idx % transition_count;
			}
		)};
		typedef tuple_type <std::remove_cvref_t <decltype(indices)>>::type transitions_type;
	};
	
	
	template <typename t_initial_state, typename... t_others>
	struct transition_list_with_to_any_other
	{
		// FIXME: Use the same approach as in transitions_between_any.
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
		typedef tuples::map_t <
			tuples::filter_t <
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
	
	struct transition_key
	{
		double		probability_threshold{};
		node_type	node{};
		
		constexpr auto as_tuple() const { return std::make_tuple(node, probability_threshold); }
		constexpr bool operator<(transition_key const &other) const { return as_tuple() < other.as_tuple(); }
		constexpr bool operator==(transition_key const &other) const { return as_tuple() == other.as_tuple(); }
	};
	
	template <typename t_initial_state, typename t_transitions>
	struct transition_map_builder
	{
		constexpr static auto const transition_count{std::tuple_size_v <t_transitions>};
		
		typedef tuples::map_t <t_transitions, transition_src_t>	src_nodes_type;
		typedef tuples::map_t <t_transitions, transition_dst_t>	dst_nodes_type;
		typedef tuples::cat_t <src_nodes_type, dst_nodes_type>	transition_nodes_type;
		typedef std::pair <transition_key, node_type>			value_type;
		
		template <std::size_t t_idx> struct index {};
		template <node_type t_idx> struct node_index {};
		
		consteval static auto const make_node_indices()
		{
			constexpr auto const size(std::tuple_size_v <transition_nodes_type>);
			std::array <node_type, size> retval;
			auto const cb([size, &retval]<std::size_t t_transition_idx, node_type t_node>(auto &&cb, index <t_transition_idx>, node_index <t_node>) constexpr {
				if constexpr (t_transition_idx < size)
				{
					typedef std::tuple_element_t <t_transition_idx, transition_nodes_type> node_type;
					constexpr auto const first_idx{tuples::first_index_of_v <transition_nodes_type, node_type>};
					retval[t_transition_idx] = first_idx;
					constexpr bool const should_increment{first_idx == t_transition_idx};
					cb(cb, index <1 + t_transition_idx>{}, node_index <should_increment + t_node>{});
				}
			});
			
			cb(cb, index <0>{}, node_index <0>{});
			return retval;
		}
		
		constexpr static auto const node_indices{make_node_indices()};
		
		consteval static auto const make_unique_node_indices()
		{
			constexpr auto const res([]() consteval {
				std::array sorted{node_indices};
				std::sort(sorted.begin(), sorted.end());
				auto const last(std::unique(sorted.begin(), sorted.end()));
				return std::make_pair(sorted, std::distance(sorted.begin(), last));
			}());
			std::array <node_type, res.second> retval;
			std::copy_n(res.first.begin(), res.second, retval.begin());
			return retval;
		}
		
		constexpr static auto const unique_node_indices{make_unique_node_indices()};
		constexpr static auto const node_limit{std::max_element(node_indices.begin(), node_indices.end())};
		constexpr static auto const initial_state{node_indices[tuples::first_index_of_v <transition_nodes_type, t_initial_state>]};
		
		consteval static auto node_tuple_helper()
		{
			auto const cb([]<std::size_t t_idx>(auto &&cb, index <t_idx>) constexpr {
				if constexpr (t_idx < unique_node_indices.size())
				{
					auto const node_idx{std::get <t_idx>(unique_node_indices)};
					return std::tuple_cat(
						std::declval <
							std::tuple <
								std::tuple_element_t <node_idx, transition_nodes_type>
							>
						>(),
						cb(cb, index <1 + t_idx>{})
					);
				}
				else
				{
					return std::tuple <>{};
				}
			});
			return cb(cb, index <0>{});
		}
		
		typedef std::invoke_result_t <decltype(&node_tuple_helper)> nodes_type;
		
		consteval static auto make_transition_map()
		{
			auto const transition_count{std::tuple_size_v <t_transitions>};
			std::array <value_type, transition_count> retval;
			
			for (std::size_t i{}; i < transition_count; ++i)
			{
				auto const src_node{node_indices[i]};
				auto const dst_node{node_indices[i + transition_count]};
				retval[i] = value_type{transition_key{0.0, src_node}, dst_node};
				
				if (i)
				{
					if (retval[i - 1].first.node == src_node)
						retval[i].first.probability_threshold += retval[i - 1].first.probability_threshold;
					else
					{
						// FIXME: Add assertion like this. (Currently Clang++17 reports that the function does not produce a constant expression.)
						//libbio_assert(compare_fp(1.0, retval[i - 1].first.probability_threshold, 0.000001)); // FIXME: better epsilon?
					}
				}
			}
			
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
		typedef aggregate <t_traits...>								traits_type;
		
	public:
		typedef t_initial_state										initial_state_type;
		typedef typename t_transitions::transitions_type			transitions_type;
		typedef detail::node_type									node_type;
		
		constexpr static auto const NODE_MAX{std::numeric_limits <node_type>::max()}; // Max. value of node_type.
		
		typedef detail::transition_map_builder <
			t_initial_state,
			transitions_type
		>															transition_map_builder_type;
		typedef typename transition_map_builder_type::value_type	transition_map_value_type;
		typedef typename transition_map_builder_type::nodes_type	nodes_type;
		
		constexpr static bool const							uses_runtime_polymorphism{std::is_base_of_v <uses_runtime_polymorphism_trait, traits_type>};
		static_assert(!uses_runtime_polymorphism || std::is_base_of_v <t_base, t_initial_state>);
		static_assert(!uses_runtime_polymorphism || detail::transition_states_have_base_v <t_base, transitions_type>);
		
	private:
		// The actual chain.
		constexpr static auto const	initial_state{transition_map_builder_type::initial_state};
		constexpr static auto const	transitions{transition_map_builder_type::make_transition_map()};
		
	public:
		typedef std::vector <
			std::conditional_t <
				uses_runtime_polymorphism,
				std::unique_ptr <t_base>,
				t_base
			>
		>															values_type;
		
		values_type													values{};
		
		template <typename t_probabilities, typename t_visitor>
		static void visit_node_types(t_probabilities &&probabilities, t_visitor &&visitor)
		{
			typedef detail::transition_key transition_key;
			
			auto const &fns(detail::callback_table_builder <t_visitor, nodes_type>::fns);
			node_type current_node{initial_state};
			
			libbio_assert_lt(current_node, fns.size());
			if (!(visitor.*(fns[current_node]))())
				return;
			
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
				if (!(visitor.*(fns[current_node]))())
					return;
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
				return true;
			});
			return retval;
		}
	};
}

#endif

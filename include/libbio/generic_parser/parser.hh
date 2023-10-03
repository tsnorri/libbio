/*
 * Copyright (c) 2022-2023 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_GENERIC_PARSER_PARSER_HH
#define LIBBIO_GENERIC_PARSER_PARSER_HH

#include <libbio/generic_parser/fields.hh>
#include <libbio/generic_parser/iterators.hh>
#include <libbio/tuple.hh>
#include <tuple>
#include <type_traits>							// std::integral_constant, std::is_same_v
#include <variant>


namespace libbio::parsing::detail {
	
	template <typename t_type>
	struct make_tuple
	{
		typedef std::tuple <t_type> type;
	};
	
	template <typename t_type>
	using make_tuple_t = typename make_tuple <t_type>::type;
	
	
	template <typename t_lhs, typename t_rhs>
	using tuple_cat_2_t = tuples::cat_t <t_lhs, t_rhs>;
	
	
	template <typename t_tuple, bool t_should_copy>
	struct included_parser_field_indices
	{
		template <std::size_t t_i>
		using size_constant = std::integral_constant <std::size_t, t_i>;
		
		
		template <std::size_t t_i>
		constexpr static auto filter_parsers()
		{
			if constexpr (t_i == std::tuple_size_v <t_tuple>)
				return std::tuple{};
			else
			{
				typedef std::tuple_element_t <t_i, t_tuple>	parser_type;
				if constexpr (std::is_same_v <void, typename parser_type::template value_type <t_should_copy>>)
					return filter_parsers <1 + t_i>();
				else
					return std::tuple_cat(std::make_tuple(size_constant <t_i>()), filter_parsers <1 + t_i>());
			}
		}
		
		using type = std::invoke_result_t <decltype(&included_parser_field_indices::filter_parsers <0>)>;
	};
	
	
	// Predicate for buffer usage.
	template <typename t_value>
	struct uses_buffer : public std::bool_constant <!std::is_trivially_copyable_v <t_value>> {};
	
	template <typename t_value>
	constexpr inline auto const uses_buffer_v{uses_buffer <t_value>::value};
	
	
	template <typename, bool, typename>
	struct parsed_type {};
	
	template <typename t_field_tuple, bool t_should_copy, typename... t_indices>
	struct parsed_type <t_field_tuple, t_should_copy, std::tuple <t_indices...>>
	{
		// Returns a tuple of tuples (s.t. the outer tuple contains at least one element)
		// where the inner tuples represent the different record types.
		template <std::size_t t_i = 0, typename t_acc = std::tuple <std::tuple <>>>
		consteval static auto helper()
		{
			static_assert(1 <= std::tuple_size_v <t_acc>);
			
			typedef std::tuple <t_indices...> index_tuple;
			constexpr auto const index_count{std::tuple_size_v <index_tuple>};
			
			if constexpr (t_i == index_count)
			{
				if constexpr (1 < std::tuple_size_v <t_acc>)
					return to_alternative_t <tuples::unique_t <t_acc>>{};
				else
					return std::tuple_element_t <0, t_acc>{};
			}
			else
			{
				typedef std::tuple_element_t <t_i, index_tuple> idx_type;						// Current index.
				constexpr auto const idx{idx_type::value};
				
				typedef std::tuple_element_t <idx, t_field_tuple> field_type;					// Current field
				typedef typename field_type::template value_type <t_should_copy> value_type;	// value type
				
				if constexpr (is_alternative_v <value_type>)
				{
					typedef typename value_type::tuple_type	value_tuple_type_;
					typedef tuples::erase_t <
						value_tuple_type_,
						std::tuple <>
					>										value_tuple_type;					// Remove empty tuple if present.
					typedef tuples::cross_product_t <
						t_acc,
						value_tuple_type,
						tuple_cat_2_t
					>										new_value_tuple_type;				// Calculate the cross product and concatenate.
					return helper <1 + t_i, new_value_tuple_type>();							// Recurse.
				}
				else
				{
					typedef tuples::cross_product_t <
						t_acc,
						std::tuple <std::tuple <value_type>>,
						tuple_cat_2_t
					>										new_value_tuple_type;				// Concatenate; could use map instead of cross product.
					return helper <1 + t_i, new_value_tuple_type>();							// Recurse.
				}
			}
		}
		
		typedef std::invoke_result_t <decltype(&parsed_type::helper <>)> type;
	};
	
	
	// Helper for counting the occurrences of each type.
	template <typename t_type, std::size_t t_count = 0>
	struct counter
	{
		typedef t_type type;
		constexpr static inline auto const count{t_count};
		
		typedef counter <t_type, 1 + count>	add_one_type;
		typedef std::array <t_type, count>	array_type;
		
		template <typename t_other_type>
		constexpr static inline auto const eq_v{std::is_same_v <t_type, t_other_type>};
		
		template <typename t_other_counter>
		struct max
		{
			typedef counter type;
		};
		
		template <typename t_other_type, std::size_t t_other_count>
		struct max <counter <t_other_type, t_other_count>>
		{
			static_assert(0 == t_other_count, "This template should only be used for overriding t_count");
			typedef counter type;
		};
		
		template <std::size_t t_other_count>
		struct max <counter <t_type, t_other_count>>
		{
			typedef counter <t_type, (t_other_count < t_count ? t_count : t_other_count)> type;
		};
		
		template <typename t_counter>
		using max_t = typename max <t_counter>::type;
	};
	
	template <typename t_counter>
	using counter_type_t = typename t_counter::type;
	
	template <typename t_counter, typename t_other_counter>
	using counter_max_t = typename t_other_counter::template max_t <t_counter>; // t_other_counter is always non-void when using foldl_t.
	
	// Helper for using counter::eq_v s.t. counter may be varied.
	template <typename t_type>
	struct counter_eq
	{
		template <typename t_counter>
		struct with : public std::bool_constant <t_counter::template eq_v <t_type>> {};
	};
	
	
	template <typename>
	struct buffer_type
	{
		// Define a dummy type to make the compiler happy.
		typedef struct {} type;
	};
	
	
	template <typename... t_tuples>
	struct buffer_type <alternative <t_tuples...>>
	{
		// Callback for counting the instances of the types.
		// Finds a counter matching t_type from t_acc, or adds a new counter.
		// The resulting counter is then added to the list (rest_type).
		template <typename t_acc, typename t_type, typename t_find_result =
			tuples::find_if <
				t_acc,
				counter_eq <t_type>::template with,
				counter <t_type>						// New counter if no matches are found.
			>
		>
		using count_callback_t = tuples::prepend_t <
			typename t_find_result::first_match_type::add_one_type,
			typename t_find_result::mismatches_type
		>;
		
		template <typename t_tuple>
		using map_filter_t = tuples::filter_t <t_tuple, uses_buffer>;
		
		template <typename t_tuple>
		using map_count_t = tuples::foldl_t <
			count_callback_t,
			tuples::empty,
			t_tuple
		>;
		
		template <typename t_tuple>
		using map_max_t = tuples::foldl_t <counter_max_t, void, t_tuple>;
		
		template <typename t_counter>
		using to_array_t = typename t_counter::array_type;
		
		// We only consider non-trivially copyable types.
		typedef std::tuple <t_tuples...>														tuple_type;
		static_assert(!tuples::find <tuple_type, std::tuple <>>::found, "The alternative should not contain std::tuple <>.");
		typedef tuples::map_t <tuple_type, map_filter_t>										filtered_types_by_tuple;	// Filter by uses_buffer.
		typedef tuples::map_t <filtered_types_by_tuple, map_count_t>							counts_by_tuple;			// Tuple of tuples of counts.
		typedef tuples::group_by_type_t <tuples::cat_with_t <counts_by_tuple>, counter_type_t>	counts_by_type;				// Group by the type.
		typedef tuples::map_t <counts_by_type, map_max_t>										max_counts;					// Max. counts.
		typedef tuples::map_t <max_counts, to_array_t>											type;						// The complete buffer type.
	};
	
	
	template <bool t_parsed_type_is_alternative, typename t_parsed_type>
	struct record_type
	{
		typedef t_parsed_type	type;
	};
	
	template <typename t_parsed_type>
	struct record_type <true, t_parsed_type>
	{
		typedef libbio::tuples::reusable_tuple <
			t_parsed_type::max_size,
			t_parsed_type::max_alignment
		> type;
	};
	
	
	// Predicate for comparing a type to the type of an std::array.
	template <typename t_type>
	struct matches_array
	{
		template <typename>
		struct with {};
		
		template <typename t_other_type, std::size_t t_size>
		struct with <std::array <t_other_type, t_size>> : public std::bool_constant <std::is_same_v <t_type, t_other_type>> {};
	};
	
	
	template <typename t_tuple, bool t_should_copy>
	using included_parser_field_indices_t = typename included_parser_field_indices <t_tuple, t_should_copy>::type;
	
	template <typename t_field_tuple, bool t_should_copy, typename t_indices>
	using parsed_type_t = typename parsed_type <t_field_tuple, t_should_copy, t_indices>::type;
	
	template <typename t_type>
	using buffer_type_t = typename buffer_type <t_type>::type;
	
	template <bool t_parsed_type_is_alternative, typename t_parsed_type>
	using record_type_t = typename record_type <t_parsed_type_is_alternative, t_parsed_type>::type;
}


namespace libbio::parsing {
	
	template <typename t_iterator, typename t_sentinel>
	auto make_range(t_iterator &&it, t_sentinel &&sentinel)
	requires std::contiguous_iterator <std::remove_cvref_t <t_iterator>>
	{
		return detail::range{
			std::forward <t_iterator>(it),
			std::forward <t_sentinel>(sentinel)
		};
	}
	
	template <typename t_iterator, typename t_sentinel, typename t_callback>
	auto make_range(t_iterator &&it, t_sentinel &&sentinel, t_callback &&callback)
	requires std::contiguous_iterator <std::remove_cvref_t <t_iterator>>
	{
		return detail::updatable_range{
			std::forward <t_iterator>(it),
			std::forward <t_sentinel>(sentinel),
			std::forward <t_callback>(callback)
		};
	}
	
	// If t_iterator is contiguous, we can determine the character position in O(1) time, but otherwise
	// we want to maintain the numeric character position. This is especially useful in case the iterator
	// is single pass.
	
	template <typename t_iterator, typename t_sentinel>
	auto make_range(t_iterator &&it, t_sentinel &&sentinel)
	requires (!std::contiguous_iterator <std::remove_cvref_t <t_iterator>>)
	{
		return detail::range{
			detail::counting_iterator(std::forward <t_iterator>(it)),
			detail::counting_iterator(std::forward <t_sentinel>(sentinel))
		};
	}
	
	
	template <typename t_iterator, typename t_sentinel, typename t_callback>
	auto make_range(t_iterator &&it, t_sentinel &&sentinel, t_callback &&cb)
	requires (!std::contiguous_iterator <std::remove_cvref_t <t_iterator>>)
	{
		// Also fix the callback.
		return detail::updatable_range{
			detail::counting_iterator(std::forward <t_iterator>(it)),
			detail::counting_iterator(std::forward <t_sentinel>(sentinel)),
			[cb_ = std::tuple <t_callback>(std::forward <t_callback>(cb))](auto &it, auto &sentinel){ // Handle rvalue references by wrapping in a tuple.
				return std::get <0>(cb_)(it.base_reference(), sentinel.base_reference());
			}
		};
	}
	
	
	template <typename t_traits, bool t_should_copy, typename... t_args>
	class parser_tpl
	{
		template <typename, bool, typename...>
		friend class parser_tpl;
		
		template <typename, typename...>
		friend struct fields::conditional;
		
	public:
		typedef std::tuple <t_args...>									field_tuple;
		typedef detail::included_parser_field_indices_t <
			field_tuple,
			t_should_copy
		>																included_field_indices;
		typedef detail::parsed_type_t <
			field_tuple,
			t_should_copy,
			included_field_indices
		>																parsed_type;
		typedef detail::buffer_type_t <parsed_type>						buffer_type;
		
		constexpr static inline auto const								parsed_type_is_alternative{is_alternative_v <parsed_type>};
		constexpr static inline auto const								field_count{std::tuple_size_v <field_tuple>};
		
		typedef typename t_traits::template trait <field_count>			trait_type;
		
		typedef typename detail::record_type_t <
			parsed_type_is_alternative,
			parsed_type
		>																record_type;
		
	protected:
		template <typename t_parser, std::size_t t_field_idx>
		struct stack_item
		{
			typedef t_parser	parser_type;
			constexpr static inline auto const field_index{t_field_idx};
		};
		
		
		template <std::size_t t_idx, typename t_value, typename t_buffer>
		static inline void swap_buffers(t_value &value, t_buffer &buffer)
		{
			typedef tuples::find_if <t_buffer, detail::matches_array <t_value>::template with>	find_result_type;
			static_assert(find_result_type::found);
			typedef typename find_result_type::first_match_type									buffer_array_type;
			
			auto &buffer_array(std::get <buffer_array_type>(buffer));	// Get the array for t_value.
			auto &buffer_item(std::get <t_idx>(buffer_array));			// Get the element that corresponds to the current tuple element.
			
			using std::swap;
			swap(value, buffer_item);
		}
		
		
		template <std::size_t t_field_idx = 0, typename t_stack = std::tuple <>, typename t_range, typename t_dst, typename t_buffer, typename t_parse_cb>
		static inline bool parse_alternatives(t_range &range, t_dst &dst, t_buffer &buffer, t_parse_cb &parse_cb, bool const did_repeat = false)
		{
			if constexpr (t_field_idx == field_count)
			{
				// Check if the parser stack (contains stack_items) is empty.
				if constexpr (0 == std::tuple_size_v <t_stack>)
				{
					// Nothing more to parse.
					parse_cb(dst);
					
					// Save the buffers if needed.
					tuples::for_ <t_dst::tuple_size>([&dst, &buffer]<typename t_idx>(){
						constexpr auto const idx{t_idx::value};
						typedef std::tuple_element_t <idx, typename t_dst::tuple_type>	value_type;
						if constexpr (detail::uses_buffer_v <value_type>)
						{
							constexpr auto const rank{tuples::rank_v <typename t_dst::tuple_type, idx>};	// Rank of the current type among items of the same type.
							auto &value(dst.template get <idx>());
							swap_buffers <rank>(value, buffer);
						}
					});
					
					// Empty here since the exact type is known and we don’t need to go through the function pointer.
					dst.clear();
					
					return true;
				}
				else
				{
					// If not, resume parsing from the previous frame.
					typedef tuples::head_t <t_stack>				stack_head_type;
					typedef tuples::tail_t <t_stack>				stack_tail_type;
					typedef typename stack_head_type::parser_type	parser_type;
					constexpr auto const field_index{stack_head_type::field_index};
					return parser_type::template parse_alternatives <field_index, stack_tail_type>(range, dst, buffer, parse_cb);
				}
			}
			else
			{
				constexpr auto const field_position{trait_type::template field_position <t_field_idx>};
				typedef std::tuple_element_t <t_field_idx, field_tuple>				field_type;
				typedef typename trait_type::template delimiter <t_field_idx>		delimiter;
				typedef typename field_type::template value_type <t_should_copy>	value_type;
				
				field_type field;
				
				// Check if the value of the current field (1) is a variant, or (2) void, i.e. not saved.
				if constexpr (is_alternative_v <value_type>)
				{
					// Push the current parser type s.t. parsing may be continued after the current field.
					typedef tuples::prepend_t <stack_item <parser_tpl, 1 + t_field_idx>, t_stack>	stack_type;
					return field.template parse <delimiter, field_position, stack_type>(range, dst, buffer, parse_cb);
				}
				else
				{
					auto do_continue([&](parse_result const &res){
						if (any(field_position & field_position::repeating_) && res)
						{
							if (0 == res.matched_delimiter_index) // We assume that the first delimiter indicates repeating the current field.
								return parse_alternatives <t_field_idx, t_stack>(range, dst, buffer, parse_cb);
							else
								return parse_alternatives <1 + t_field_idx, t_stack>(range, dst, buffer, parse_cb);
						}
						else if (any(field_position & field_position::initial_) && !res)
						{
							return false;
						}
						else
						{
							return parse_alternatives <1 + t_field_idx, t_stack>(range, dst, buffer, parse_cb);
						}
					});

					if constexpr (std::is_same_v <void, value_type>)
					{
						// Parse but do not save.
						auto const res(field.template parse <delimiter, field_position>(range));
						return do_continue(res);
					}
					else
					{
						// Parse and save.
						auto do_continue_([&](auto &value){
							auto const res(field.template parse <delimiter, field_position>(range, value));
							return do_continue(res);
						});

						if (any(field_position & field_position::repeating_) && did_repeat)
						{
							// Use the existing value.
							auto &value(dst.back());
							return do_continue_(value);
						}
						else
						{
							// Instantiate a new value.
							auto &dst_(dst.template append <value_type>()); // Returns dst cast to a new type.
							auto &value(dst_.back());
							
							// Swap the buffers if needed.
							if constexpr (detail::uses_buffer_v <value_type>)
							{
								// Calculate the rank using t_dst.
								typedef std::remove_cvref_t <decltype(dst_)> new_dst_type;
								typedef typename new_dst_type::tuple_type dst_tuple_type;
								constexpr auto const rank{tuples::rank_v <dst_tuple_type, std::tuple_size_v <dst_tuple_type> - 1>};
								swap_buffers <rank>(value, buffer);
							}

							return do_continue_(value);
						}
					}
				}
			}
		}
		
		
		template <typename t_range, typename t_dst, typename t_parse_cb>
		inline bool parse__(t_range &range, t_dst &dst, buffer_type &buffer, t_parse_cb &parse_cb)
		requires parsed_type_is_alternative
		{
			// Recurse to build the tuple for the current record.
			return parse_alternatives(range, dst, buffer, parse_cb);
		}
		
		
		template <std::size_t t_field_idx = 0, std::size_t t_value_idx = 0, typename t_range>
		inline bool parse__(t_range &range, record_type &dst)
		requires (!parsed_type_is_alternative)
		{
			if constexpr (t_field_idx == field_count)
				return true;
			else
			{
				typedef std::tuple_element_t <t_field_idx, field_tuple>			field_type;
				typedef typename trait_type::template delimiter <t_field_idx>	delimiter;
				template <std::size_t t_idx> using index_type = std::integral_constant <std::size_t, t_idx>;

				constexpr auto const field_position{trait_type::template field_position <t_field_idx>};
				field_type field;
				
				auto do_continue([&]<typename t_next_value_idx>(parse_result const &res, t_next_value_idx const &){
					if (any(field_position & field_position::repeating_) && res)
					{
						if (0 == res.matched_delimiter_index) // We assume that the first delimiter indicates repeating the current field.
							return parse__ <t_field_idx, t_next_value_idx>(range, dst);
						else
							return parse__ <1 + t_field_idx, t_next_value_idx>(range, dst);
					}
					else if (any(field_position & field_position::final_) && !res) // FIXME: incorrect?
					{
						return false;
					}
					else
					{
						return parse__ <1 + t_field_idx, t_next_value_idx>(range, dst);
					}
				});

				// Check if the value of the current field should be saved.
				if constexpr (std::is_same_v <void, typename field_type::template value_type <t_should_copy>>)
				{
					auto const res(field.template parse <delimiter, field_position>(range));
					return do_continue(res, index_type <t_value_idx>{}); // Skip instead.
				}
				else
				{
					auto const res(field.template parse <delimiter, field_position>(range, std::get <t_value_idx>(dst)));
					return do_continue(res, index_type <1 + t_value_idx>{}); // Save.
				}
			}
		}
		
		
		template <typename t_range, typename... t_args_>
		inline bool parse_(t_range &range, t_args_ && ... args)
		{
			try
			{
				return parse__(range, std::forward <t_args_>(args)...);
			}
			catch (parse_error &error)
			{
				error.m_position = range.position();
				throw;
			}
		}
		
		
	public:
		// User supplies the range.
		// FIXME: require that the range is one of detail::range, detail::updatable_range.
		template <typename t_range>
		bool parse(t_range &range, record_type &dst)
		requires (!parsed_type_is_alternative)
		{
			return parse_(range, dst);
		}
		
		
		template <typename t_range, std::size_t t_max_size, std::size_t t_alignment, typename... t_args_, typename t_parse_cb>
		bool parse(
			t_range &range,
			tuples::reusable_tuple <t_max_size, t_alignment, t_args_...> &dst,
			buffer_type &buffer,
			t_parse_cb &&parse_cb
		)
		requires (
			parsed_type_is_alternative &&
			tuples::reusable_tuple <t_max_size, t_alignment, t_args_...>::template can_fit_v <record_type>
		)
		{
			return parse_(range, dst, buffer, parse_cb);
		}
		
		
		// Non-updatable range, e.g. std::istream_iterator, memory mapped file.
		template <typename t_iterator, typename t_sentinel>
		bool parse(t_iterator &&it, t_sentinel &&sentinel, record_type &dst)
		requires (!parsed_type_is_alternative)
		{
			auto range(make_range(
				std::forward <t_iterator>(it),
				std::forward <t_sentinel>(sentinel)
			));
			return parse_(range, dst);
		}
		
		
		template <
			typename t_iterator,
			typename t_sentinel,
			std::size_t t_max_size,
			std::size_t t_alignment,
			typename... t_args_,
			typename t_parse_cb
		>
		bool parse(
			t_iterator &&it,
			t_sentinel &&sentinel,
			tuples::reusable_tuple <t_max_size, t_alignment, t_args_...> &dst,
			buffer_type &buffer,
			t_parse_cb &&parse_cb
		)
		requires (
			parsed_type_is_alternative &&
			tuples::reusable_tuple <t_max_size, t_alignment, t_args_...>::template can_fit_v <record_type>
		)
		{
			auto range(make_range(
				std::forward <t_iterator>(it),
				std::forward <t_sentinel>(sentinel)
			));
			return parse_(range, dst, buffer, parse_cb);
		}
		
		
		// Updatable range, e.g. input read in blocks.
		template <typename t_iterator, typename t_sentinel, typename t_update_cb>
		bool parse(t_iterator &&it, t_sentinel &&sentinel, record_type &dst, t_update_cb &&cb)
		requires (!parsed_type_is_alternative)
		{
			auto range(make_range(
				std::forward <t_iterator>(it),
				std::forward <t_sentinel>(sentinel),
				std::forward <t_update_cb>(cb)
			));
			return parse_(range, dst);
		}
		
		
		template <
			typename t_iterator,
			typename t_sentinel,
			std::size_t t_max_size,
			std::size_t t_alignment,
			typename... t_args_,
			typename t_update_cb,
			typename t_parse_cb
		>
		bool parse(
			t_iterator &&it,
			t_sentinel &&sentinel,
			tuples::reusable_tuple <t_max_size, t_alignment, t_args_...> &dst,
			buffer_type &buffer,
			t_update_cb &&update_cb,
			t_parse_cb &&parse_cb
		)
		requires (
			parsed_type_is_alternative &&
			tuples::reusable_tuple <t_max_size, t_alignment, t_args_...>::template can_fit_v <record_type>
		)
		{
			auto range(make_range(
				std::forward <t_iterator>(it),
				std::forward <t_sentinel>(sentinel),
				std::forward <t_update_cb>(update_cb)
			));
			return parse_(range, dst, buffer, parse_cb);
		}
		
		
		template <typename... t_args_>
		void parse_all(t_args_ && ... args)
		requires parsed_type_is_alternative
		{
			bool status{};
			do
			{
				status = parse(std::forward <t_args_>(args)...);
			} while (status);
		}
	};
	
	
	template <typename t_traits, typename... t_args>
	using parser = parser_tpl <t_traits, true, t_args...>;
	
	template <typename t_traits, typename... t_args>
	using transient_parser = parser_tpl <t_traits, false, t_args...>;
}


namespace libbio::parsing::fields {
	
	template <typename t_tag, typename... t_fields>
	struct option
	{
		template <typename t_traits>
		using tagged_parser_t = tagged_parser <t_tag, parser <t_traits, t_fields...>>;
	};
	
	
	template <typename t_base, typename t_traits, typename... t_options>
	using make_conditional = conditional <t_base, typename t_options::template tagged_parser_t <t_traits>...>;
}

#endif

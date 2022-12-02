/*
 * Copyright (c) 2022 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_GENERIC_PARSER_PARSER_HH
#define LIBBIO_GENERIC_PARSER_PARSER_HH

#include <libbio/generic_parser/iterators.hh>
#include <tuple>
#include <type_traits>							// std::integral_constant, std::is_same_v


namespace libbio::parsing::detail {
		
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
	
	
	template <typename, bool, typename>
	struct record_type {};
	
	template <typename t_parser_tuple, bool t_should_copy, typename... t_indices>
	struct record_type <t_parser_tuple, t_should_copy, std::tuple <t_indices...>>
	{
		typedef std::tuple <
			typename std::tuple_element_t <
				t_indices::value,
				t_parser_tuple
			>::template value_type <t_should_copy>...
		>	type;
	};
	
	
	template <typename t_tuple, bool t_should_copy>
	using included_parser_field_indices_t = typename included_parser_field_indices <t_tuple, t_should_copy>::type;
	
	
	template <typename t_parser_tuple, bool t_should_copy, typename t_indices>
	using record_type_t = typename record_type <t_parser_tuple, t_should_copy, t_indices>::type;
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
	public:
		typedef std::tuple <t_args...>									field_tuple;
		typedef detail::included_parser_field_indices_t <
			field_tuple,
			t_should_copy
		>																included_field_indices;
		typedef detail::record_type_t <
			field_tuple,
			t_should_copy,
			included_field_indices
		>																record_type;
		constexpr static inline std::size_t const						field_count{std::tuple_size_v <field_tuple>};
		constexpr static inline std::size_t const						value_count{std::tuple_size_v <record_type>};
		typedef typename t_traits::template trait <field_count>			trait_type;
		
	protected:
		template <std::size_t t_field_idx, std::size_t t_value_idx, typename t_range>
		inline bool parse__(t_range &range, record_type &dst)
		{
			if constexpr (t_field_idx == field_count)
				return true;
			else
			{
				typedef std::tuple_element_t <t_field_idx, field_tuple>			field_type;
				typedef typename trait_type::template delimiter <t_field_idx>	delimiter;
				constexpr auto const field_position{trait_type::template field_position <t_field_idx>};
				
				field_type field;
				
				// Check if the value of the current field should be saved.
				if constexpr (std::is_same_v <void, typename field_type::template value_type <t_should_copy>>)
				{
					auto const status(field.template parse <delimiter, field_position>(range));
					if (any(field_position & field_position::final_) && !status)
						return false;
					else
						return parse__ <1 + t_field_idx, t_value_idx>(range, dst);
				}
				else
				{
					auto const status(field.template parse <delimiter, field_position>(range, std::get <t_value_idx>(dst)));
					if (any(field_position & field_position::final_) && !status)
						return false;
					else
						return parse__ <1 + t_field_idx, 1 + t_value_idx>(range, dst);
				}
			}
		}
		
		
		template <typename t_range>
		inline bool parse_(t_range &range, record_type &dst)
		{
			try
			{
				return parse__ <0, 0>(range, dst);
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
		{
			return parse_(range, dst);
		}
		
		
		// Non-updatable range, e.g. std::istream_iterator, memory mapped file.
		template <typename t_iterator, typename t_sentinel>
		bool parse(t_iterator &&it, t_sentinel &&sentinel, record_type &dst)
		{
			auto range(make_range(
				std::forward <t_iterator>(it),
				std::forward <t_sentinel>(sentinel)
			));
			return parse_(range, dst);
		}
		
		
		// Updatable range, e.g. input read in blocks.
		template <typename t_iterator, typename t_sentinel, typename t_update_cb>
		bool parse(t_iterator &&it, t_sentinel &&sentinel, record_type &dst, t_update_cb &&cb)
		{
			auto range(make_range(
				std::forward <t_iterator>(it),
				std::forward <t_sentinel>(sentinel),
				std::forward <t_update_cb>(cb)
			));
			return parse_(range, dst);
		}
	};
	
	
	template <typename t_traits, typename... t_args>
	using parser = parser_tpl <t_traits, true, t_args...>;
	
	template <typename t_traits, typename... t_args>
	using transient_parser = parser_tpl <t_traits, false, t_args...>;
}

#endif

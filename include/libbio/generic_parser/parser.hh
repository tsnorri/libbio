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
	
	template <typename t_traits, bool t_should_copy, typename... t_args>
	class parser
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
				typedef std::tuple_element_t <t_field_idx, field_tuple>			parser_type;
				typedef typename trait_type::template delimiter <t_field_idx>	delimiter;
				constexpr auto const field_position{trait_type::template field_position <t_field_idx>};
				
				parser_type parser;
				
				// Check if the value of the current field should be saved.
				if constexpr (std::is_same_v <void, typename parser_type::template value_type <t_should_copy>>)
				{
					auto const status(parser.template parse <delimiter, field_position>(range));
					if (any(field_position & field_position::final_) && !status)
						return false;
					else
						return parse__ <1 + t_field_idx, t_value_idx>(range, dst);
				}
				else
				{
					auto const status(parser.template parse <delimiter, field_position>(range, std::get <t_value_idx>(dst)));
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
		
		
		// Helper for conditionally wrapping the iterators.
		template <template <typename...> typename t_range, typename t_iterator, typename t_sentinel>
		struct helper
		{
			typedef detail::counting_iterator <t_iterator>	counting_iterator;
			typedef detail::counting_iterator <t_sentinel>	counting_sentinel;
			
			template <typename... t_args_>
			static inline bool parse(parser &parser, t_iterator &it, t_sentinel const sentinel, record_type &dst, t_args_ && ... args)
			requires std::contiguous_iterator <t_iterator>
			{
				t_range range{it, sentinel, std::forward <t_args_>(args)...};
				return parser.parse_(range, dst);
			}
			
			// If t_iterator is contiguous, we can determine the character position in O(1) time, but otherwise
			// we want to maintain the numeric character position. This is especially useful in case the iterator
			// is single pass.
			
			static inline bool parse(parser &parser, t_iterator &it, t_sentinel const sentinel, record_type &dst)
			requires (!std::contiguous_iterator <t_iterator>)
			{
				counting_iterator it_(it);
				counting_sentinel sentinel_(sentinel);
				t_range range{it_, sentinel_};
				return parser.parse_(range, dst);
			}
			
			template <typename t_callback>
			static inline bool parse(parser &parser, t_iterator &it, t_sentinel const sentinel, record_type &dst, t_callback &cb)
			requires (!std::contiguous_iterator <t_iterator>)
			{
				counting_iterator it_(it);
				counting_sentinel sentinel_(sentinel);
				
				// Also fix the callback.
				auto cb_([&cb](auto &it, auto &sentinel){ return cb(it.base_reference(), sentinel.base_reference()); });
				
				t_range range{it_, sentinel_, cb_};
				return parser.parse_(range, dst);
			}
		};
		
		template <template <typename...> typename t_range, typename t_iterator, typename t_sentinel>
		struct helper <t_range, detail::counting_iterator <t_iterator>, detail::counting_iterator <t_sentinel>>
		{
			template <typename... t_args_>
			static inline bool parse(parser &parser, t_iterator &it, t_sentinel const sentinel, record_type &dst, t_args_ && ... args)
			{
				t_range range{it, sentinel, std::forward <t_args_>(args)...};
				return parser.parse_(range, dst);
			}
		};
		
		
	public:
		// Non-updatable range, e.g. std::istream_iterator, memory mapped file.
		template <typename t_iterator, typename t_sentinel>
		bool parse(t_iterator &&it, t_sentinel const sentinel, record_type &dst)
		{
			typedef std::remove_cvref_t <t_iterator>	iterator_type;
			typedef std::remove_cvref_t <t_sentinel>	sentinel_type;
			return helper <detail::range, iterator_type, sentinel_type>::parse(*this, it, sentinel, dst);
		}
		
		
		// Updatable range, e.g. input read in blocks.
		template <typename t_iterator, typename t_sentinel, typename t_update_cb>
		bool parse(t_iterator &&it, t_sentinel const sentinel, record_type &dst, t_update_cb &&cb)
		{
			typedef std::remove_cvref_t <t_iterator>	iterator_type;
			typedef std::remove_cvref_t <t_sentinel>	sentinel_type;
			return helper <detail::updatable_range, iterator_type, sentinel_type>::parse(*this, it, sentinel, dst, cb);
		}
	};
}

#endif

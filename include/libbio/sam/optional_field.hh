/*
 * Copyright (c) 2023 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_SAM_OPTIONAL_FIELD_HH
#define LIBBIO_SAM_OPTIONAL_FIELD_HH

#include <array>
#include <libbio/assert.hh>
#include <libbio/generic_parser.hh>
#include <libbio/tuple.hh>
#include <range/v3/view/take_exactly.hpp>


namespace libbio::sam::detail {
	
	template <typename t_type>
	struct is_vector : public std::false_type {};
	
	template <typename t_type>
	struct is_vector <std::vector <t_type>> : public std::true_type {};
	
	template <typename t_type>
	constexpr inline bool const is_vector_v{is_vector <t_type>::value};
	
	
	template <typename t_type, bool t_should_clear_elements = false>
	struct vector_container
	{
		std::vector <t_type>	values;
		std::size_t				size{};
		
		void clear();
		t_type &operator[](std::size_t const idx) { return values[idx]; }
		t_type const &operator[](std::size_t const idx) const { return values[idx]; }
		
		template <typename ... t_args>
		inline t_type &emplace_back(t_args && ... args);
	};
	
	
	// For using the visitor pattern with the optional fields and their values
	// we build an array of function pointers of the different overloads of the visitor.
	template <typename, typename> struct callback_table_builder {};
	
	template <typename t_callback, typename... t_args>
	struct callback_table_builder <t_callback, std::tuple <t_args...>>
	{
		// The first template paramter contains the type code (one of A, i, f, Z, H, B) and the second is the value type (e.g. char, std::vector <std::int16_t>).
		constexpr static std::array const fns{(&t_callback::template operator() <t_args>)...};
	};
	
	
	template <typename t_type, bool t_should_clear_elements>
	void vector_container <t_type, t_should_clear_elements>::clear()
	{
		if constexpr (t_should_clear_elements)
		{
			for (auto &val : values | ranges::views::take_exactly(size))
				val.clear();
		}
		
		size = 0;
	}
	
	
	template <typename t_type, bool t_should_clear_elements>
	template <typename ... t_args>
	auto vector_container <t_type, t_should_clear_elements>::emplace_back(t_args && ... args) -> t_type &
	{
		if (size < values.size())
			values[size] = std::forward <t_args...>(args...);
		else
			values.emplace_back(std::forward <t_args...>(args...));
		
		return values[size++];
	}
}


namespace libbio::sam {
	
	class optional_field
	{
		friend std::ostream &operator<<(std::ostream &, optional_field const &);
		
	public:
		template <typename t_type>
		using value_vector = std::vector <t_type>;
		
	private:
		template <typename t_type>
		using array_vector = detail::vector_container <value_vector <t_type>>;
		
		// Type mapping.
		template <typename t_type> struct container_of	{ typedef value_vector <t_type> type; };
		template <> struct container_of <std::string>	{ typedef detail::vector_container <std::string> type; };
		template <typename t_type> using container_of_t = container_of <t_type>::type;
		
		template <typename t_type>
		using value_container_t = std::conditional_t <
			detail::is_vector_v <t_type>,
			array_vector <typename t_type::value_type>,
			container_of_t <t_type>
		>;
		
	private:
		typedef std::uint16_t							tag_id_type;
		typedef std::uint16_t							tag_count_type;	// We assume that there will not be more than 65536 tags.
		struct tag_rank
		{
			// For alignment purposes all are std::uint16_t.
			tag_id_type		tag_id{};
			tag_id_type		type_index{};
			tag_count_type	rank{};
			
			bool operator<(tag_rank const &other) const { return tag_id < other.tag_id; }
		};
		
		typedef std::vector <tag_rank> tag_rank_vector;
		
		struct tag_rank_cmp
		{
			bool operator()(tag_rank const &lhs, tag_id_type const rhs) const { return lhs.tag_id < rhs; }
			bool operator()(tag_id_type const lhs, tag_rank const &rhs) const { return lhs < rhs.tag_id; }
		};
		
		// Types from SAM 1.0 Section 1.5.
		// We use std::tuple b.c. std::get can be used to get the element of a given type.
		static_assert(!std::is_same_v <std::byte, std::int8_t>); // Just to make sure.
		static_assert(!std::is_same_v <std::byte, std::uint8_t>);
		typedef std::tuple <
			container_of_t <char>,
			container_of_t <std::int32_t>,	// Type from BAM, see SAM 1.0, footnote 16.
			container_of_t <float>,
			container_of_t <std::string>,
			array_vector <std::byte>,
			array_vector <std::int8_t>,
			array_vector <std::uint8_t>,
			array_vector <std::int16_t>,
			array_vector <std::uint16_t>,
			array_vector <std::int32_t>,
			array_vector <std::uint32_t>,
			array_vector <float>
		> value_tuple_type;
		
		template <char t_type_code>
		using array_type_code_helper_t = std::integral_constant <char, t_type_code>;
		
	public:
		constexpr static std::array type_codes{'A', 'i', 'f', 'Z', 'H', 'B', 'B', 'B', 'B', 'B', 'B', 'B'};
		static_assert(std::tuple_size_v <value_tuple_type> == type_codes.size());
		
		template <typename t_type> struct array_type_code {};
		template <> struct array_type_code <std::int8_t>	: public array_type_code_helper_t <'c'> {};
		template <> struct array_type_code <std::uint8_t>	: public array_type_code_helper_t <'C'> {};
		template <> struct array_type_code <std::int16_t>	: public array_type_code_helper_t <'s'> {};
		template <> struct array_type_code <std::uint16_t>	: public array_type_code_helper_t <'S'> {};
		template <> struct array_type_code <std::int32_t>	: public array_type_code_helper_t <'i'> {};
		template <> struct array_type_code <std::uint32_t>	: public array_type_code_helper_t <'I'> {};
		template <> struct array_type_code <float>			: public array_type_code_helper_t <'f'> {};
		
		template <typename t_type>
		constexpr static inline auto const array_type_code_v{array_type_code <t_type>::value};
		
	private:
		tag_rank_vector		m_tag_ranks;	// Rank of the tag among its value type.
		value_tuple_type	m_values;
		
	private:
		template <typename t_type, typename t_value>
		inline void add_value(tag_id_type const tag_id, t_value &&val);
		
		template <typename t_type>
		inline void start_array(tag_id_type const tag_id);
		
		template <typename t_type, typename t_value>
		inline void add_array_value(t_value const);
		
		void update_tag_order() { std::sort(m_tag_ranks.begin(), m_tag_ranks.end()); }
		
		template <typename t_type, typename t_optional_field>
		static inline t_type *do_get(t_optional_field &&of, tag_id_type const tag);
		
	public:
		void clear() { m_tag_ranks.clear(); tuples::for_each(m_values, []<typename t_idx>(auto &element){ element.clear(); }); }
		template <typename t_type> t_type *get(tag_id_type const tag) { return do_get <t_type>(*this, tag); }
		template <typename t_type> t_type const *get(tag_id_type const tag) const { return do_get <t_type const>(*this, tag); }
		
		template <typename t_visitor>
		void visit(tag_rank const &tr, t_visitor &&visitor) const;
	};
	
	
	std::ostream &operator<<(std::ostream &os, optional_field const &of);
	
	
	template <typename t_type, typename t_value>
	void optional_field::add_value(tag_id_type const tag_id, t_value &&val)
	{
		typedef container_of_t <t_type> container_type;
		constexpr auto const idx{tuples::first_index_of_v <value_tuple_type, container_type>};
		auto &dst(std::get <container_type>(m_values));
		m_tag_ranks.emplace_back(tag_id, idx, dst.size());
		dst.emplace_back(std::forward <t_value>(val));
	}
	
	
	template <typename t_type>
	void optional_field::start_array(tag_id_type const tag_id)
	{
		typedef array_vector <t_type> container_type;
		constexpr auto const idx{tuples::first_index_of_v <value_tuple_type, container_type>};
		auto &dst(std::get <container_type>(m_values));
		m_tag_ranks.emplace_back(tag_id, idx, dst.size());
		dst.emplace_back();
	}
	
	
	template <typename t_type, typename t_value>
	void optional_field::add_array_value(t_value const val)
	{
		std::get <array_vector <t_type>>(m_values).values.back().emplace_back(val);
	}
	
	
	template <typename t_type, typename t_optional_field>
	t_type *optional_field::do_get(t_optional_field &&of, tag_id_type const tag)
	{
		typedef value_container_t <t_type> tuple_element_type;
		constexpr auto const idx{tuples::first_index_of_v <value_tuple_type, tuple_element_type>};
		auto const end(of.m_tag_ranks.end());
		auto const it(std::lower_bound(of.m_tag_ranks.begin(), end, tag, tag_rank_cmp{}));
		if (end == it) return nullptr;
		if (it->tag_id != tag) return nullptr;
		if (it->type_index != idx) return nullptr;
		return &std::get <tuple_element_type>(of.m_values)[it->rank];
	}
	
	
	template <typename t_visitor>
	void optional_field::visit(tag_rank const &tr, t_visitor &&visitor) const
	{
		auto visitor_caller([this, &visitor]<typename t_idx_type> {
			constexpr auto const idx{t_idx_type::value};
			// Pass the type code as a template parameter to the visitor.
			visitor <type_codes[idx]>(std::get <idx>(m_values));
		});
		
		typedef detail::callback_table_builder <
			decltype(visitor_caller),
			tuples::index_constant_sequence_for <value_tuple_type>
		> callback_table_type;
		
		constexpr auto const &fns(callback_table_type::fns);
		*(fns[tr.type_index])();
	}
}

#endif

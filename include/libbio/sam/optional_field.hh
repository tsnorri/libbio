/*
 * Copyright (c) 2023-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_SAM_OPTIONAL_FIELD_HH
#define LIBBIO_SAM_OPTIONAL_FIELD_HH

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <functional>						// std::not_fn
#include <libbio/algorithm.hh>
#include <libbio/assert.hh>
#include <libbio/generic_parser.hh>
#include <libbio/sam/input_range.hh>
#include <libbio/sam/tag.hh>
#include <libbio/tuple.hh>
#include <libbio/type_traits.hh>
#include <limits>
#include <ostream>
#include <range/v3/view/subrange.hpp>
#include <range/v3/view/take_exactly.hpp>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>


namespace libbio::sam::detail {

	typedef double floating_point_type;


	template <typename t_type, bool t_should_clear_elements = false>
	struct vector_container
	{
		std::vector <t_type>	values;
		std::size_t				size_{};

		std::size_t size() const { return size_; }
		void clear();
		t_type &operator[](std::size_t const idx) { return values[idx]; }
		t_type const &operator[](std::size_t const idx) const { return values[idx]; }
		t_type &back() { libbio_assert_lt(0, size_); return values[size_ - 1]; }
		t_type const &back() const { libbio_assert_lt(0, size_); return values[size_ - 1]; }

		auto begin() { return values.begin(); }
		auto begin() const { return values.begin(); }
		auto cbegin() const { return values.begin(); }

		auto end() { return values.begin() + size_; }
		auto end() const { return values.begin() + size_; }
		auto cend() const { return values.begin() + size_; }

		auto value_range() { return values | ranges::views::take_exactly(size_); }
		auto value_range() const { return values | ranges::views::take_exactly(size_); }

		template <typename ... t_args>
		inline t_type &emplace_back(t_args && ... args);
	};


	template <typename t_type>
	using value_vector = std::vector <t_type>;

	template <typename t_type>
	using array_vector = vector_container <value_vector <t_type>>;

	template <typename t_type> struct container_of	{ typedef value_vector <t_type> type; };
	template <> struct container_of <std::string>	{ typedef vector_container <std::string> type; };
	template <typename t_type> using container_of_t = container_of <t_type>::type;


	// For using the visitor pattern with the optional fields and their values
	// we build an array of function pointers of the different overloads of the visitor.
	template <typename, typename> struct callback_table_builder {};

	template <typename t_callback, typename... t_args>
	struct callback_table_builder <t_callback, std::tuple <t_args...>>
	{
		constexpr static std::array const fns{(&t_callback::template operator() <t_args>)...};
	};


	template <typename t_type, bool t_should_clear_elements>
	void vector_container <t_type, t_should_clear_elements>::clear()
	{
		if constexpr (t_should_clear_elements)
		{
			for (auto &val : values | ranges::views::take_exactly(size_))
				val.clear();
		}

		size_ = 0;
	}


	template <typename t_type, bool t_should_clear_elements>
	template <typename ... t_args>
	auto vector_container <t_type, t_should_clear_elements>::emplace_back(t_args && ... args) -> t_type &
	{
		if (size_ < values.size())
			values[size_] = t_type(std::forward <t_args>(args)...);
		else
			values.emplace_back(std::forward <t_args>(args)...);

		return values[size_++];
	}


	template <char t_type_code>
	using array_type_code_helper_t = std::integral_constant <char, t_type_code>;

	template <typename t_type> struct array_type_code {};
	template <> struct array_type_code <std::int8_t>			: public array_type_code_helper_t <'c'> {};
	template <> struct array_type_code <std::uint8_t>			: public array_type_code_helper_t <'C'> {};
	template <> struct array_type_code <std::int16_t>			: public array_type_code_helper_t <'s'> {};
	template <> struct array_type_code <std::uint16_t>			: public array_type_code_helper_t <'S'> {};
	template <> struct array_type_code <std::int32_t>			: public array_type_code_helper_t <'i'> {};
	template <> struct array_type_code <std::uint32_t>			: public array_type_code_helper_t <'I'> {};
	template <> struct array_type_code <floating_point_type>	: public array_type_code_helper_t <'f'> {};


	template <typename t_type>
	struct value_container
	{
		typedef container_of_t <t_type> type;
	};

	template <typename t_type, typename ... t_args>
	struct value_container <std::vector <t_type, t_args...>>
	{
		typedef array_vector <t_type> type;
	};

	template <typename t_type>
	using value_container_t = value_container <t_type>::type;
}


namespace libbio::bam::fields::detail {
	struct optional_helper;
}


namespace libbio::sam {

	class optional_field
	{
		friend void read_optional_fields(input_range_base &range, optional_field &dst);
		friend std::ostream &operator<<(std::ostream &, optional_field const &);
		friend void output_optional_field_in_parsed_order(std::ostream &, optional_field const &, std::vector <std::size_t> &);
		friend struct bam::fields::detail::optional_helper;

	public:
		enum class get_value_error
		{
			not_found,
			type_mismatch
		};

		typedef detail::floating_point_type	floating_point_type;

		// Type mapping.
		template <typename t_type> using array_vector				= detail::array_vector <t_type>;
		template <typename t_type> using container_of_t				= detail::container_of_t <t_type>;
		template <typename t_type> using value_container_t			= detail::value_container_t <t_type>;

		template <typename t_type> using get_value_return_type_t	= std::expected <std::reference_wrapper <t_type>, get_value_error>; // // std::expected does not currently allow reference types.

		typedef std::uint16_t										tag_count_type;	// We assume that there will not be more than 65536 tags.
		constexpr static inline tag_count_type const TAG_COUNT_MAX{std::numeric_limits <tag_count_type>::max()};
		struct tag_rank
		{
			// For alignment purposes all are std::uint16_t.
			// Since the tags are sorted by tag_id, the rank (unfortunately) needs to be stored, too
			// (i.e. calculating the rank with std::distance is not possible).
			// FIXME: since there are typically very few tags, linear search could be utilised instead.
			tag_type		tag_id{};
			tag_type		type_index{};
			tag_count_type	rank{};
			tag_count_type	parsed_rank{TAG_COUNT_MAX};

			tag_rank() = default;

			constexpr tag_rank(
				tag_type const tag_id_,
				tag_type const type_index_,
				tag_count_type const rank_,
				tag_count_type const parsed_rank_ = TAG_COUNT_MAX
			):
				tag_id(tag_id_),
				type_index(type_index_),
				rank(rank_),
				parsed_rank(parsed_rank_)
			{
			}

			bool operator<(tag_rank const &other) const { return tag_id < other.tag_id; }
			auto type_and_rank_to_tuple() const { return std::make_tuple(type_index, rank); }
		};

		typedef std::vector <tag_rank> tag_rank_vector;

		struct tag_rank_cmp
		{
			bool operator()(tag_rank const &lhs, tag_type const rhs) const { return lhs.tag_id < rhs; }
			bool operator()(tag_type const lhs, tag_rank const &rhs) const { return lhs < rhs.tag_id; }
		};

		// Types from SAM 1.0 Section 1.5.
		// We use std::tuple b.c. std::get can be used to get the element of a given type.
		static_assert(!std::is_same_v <std::byte, std::int8_t>); // Just to make sure.
		static_assert(!std::is_same_v <std::byte, std::uint8_t>);

		typedef std::tuple <
			container_of_t <char>,
			container_of_t <std::int8_t>,			// BAM only
			container_of_t <std::uint8_t>,			// BAM only
			container_of_t <std::int16_t>,			// BAM only
			container_of_t <std::uint16_t>,			// BAM only
			container_of_t <std::int32_t>,			// Type from BAM, see SAM 1.0, footnote 16.
			container_of_t <std::uint32_t>,			// BAM only
			container_of_t <floating_point_type>,
			container_of_t <std::string>,
			array_vector <std::byte>,
			array_vector <std::int8_t>,
			array_vector <std::uint8_t>,
			array_vector <std::int16_t>,
			array_vector <std::uint16_t>,
			array_vector <std::int32_t>,
			array_vector <std::uint32_t>,
			array_vector <floating_point_type>
		> value_tuple_type;

		constexpr static std::array type_codes{'A', 'c', 'C', 's', 'S', 'i', 'I', 'f', 'Z', 'H', 'B', 'B', 'B', 'B', 'B', 'B', 'B'};
		constexpr static std::array array_type_codes{'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', 'c', 'C', 's', 'S', 'i', 'I', 'f'};
		static_assert(std::tuple_size_v <value_tuple_type> == type_codes.size());
		static_assert(array_type_codes.size() == type_codes.size());

		template <typename t_type>
		constexpr static inline auto const array_type_code_v{detail::array_type_code <t_type>::value};

	private:
		template <typename t_optional_field>
		using find_rank_return_type_t = if_const_t <t_optional_field, tag_rank_vector::const_iterator, tag_rank_vector::iterator>;

	private:
		tag_rank_vector		m_tag_ranks;	// Rank of the tag among its value type.
		value_tuple_type	m_values;

	private:
		template <typename t_type>
		inline t_type &prepare_for_adding(tag_type const tag_id);

		template <typename t_type, typename t_value>
		inline void add_value(tag_type const tag_id, t_value &&val);

		inline std::string &start_string(tag_type const tag_id);
		inline std::string &current_string_value() { return std::get <container_of_t <std::string>>(m_values).back(); }

		// FIXME: Use some typedef in the return type.
		template <typename t_type>
		inline std::vector <t_type> &start_array(tag_type const tag_id);

		template <typename t_type, typename t_value>
		inline void add_array_value(t_value const);

		void update_tag_order() { std::sort(m_tag_ranks.begin(), m_tag_ranks.end()); }

		template <typename t_type, typename t_optional_field>
		constexpr static inline auto find_rank(t_optional_field &&of, tag_type const tag) -> find_rank_return_type_t <t_optional_field>;

		template <typename t_type, typename t_optional_field>
		constexpr static inline get_value_return_type_t <t_type> do_get(t_optional_field &&of, tag_rank_vector::const_iterator it, tag_type const tag);

		template <typename t_type, typename t_optional_field>
		constexpr static inline get_value_return_type_t <t_type> do_get(t_optional_field &&of, tag_type const tag);

		void erase_values_in_range(tag_rank_vector::const_iterator rank_it, tag_rank_vector::const_iterator const rank_end);

		template <typename t_predicate, typename t_callback>
		void erase_if_(t_predicate &&predicate, t_callback &&erase_callback);

		bool compare_values_strict(optional_field const &other, tag_rank const &lhsr, tag_rank const &rhsr) const;

	public:
		optional_field() = default;

		optional_field(tag_rank_vector &&tag_ranks, value_tuple_type &&values):	// For unit tests.
			m_tag_ranks(std::move(tag_ranks)),
			m_values(std::move(values))
		{
			update_tag_order();
		}

		constexpr bool empty() const { return m_tag_ranks.empty(); }
		constexpr void clear() { m_tag_ranks.clear(); tuples::for_each(m_values, []<typename t_idx>(auto &element){ element.clear(); }); }

		template <typename t_type> constexpr get_value_return_type_t <t_type> get(tag_type const tag) { return do_get <t_type>(*this, tag); }
		template <typename t_type> constexpr get_value_return_type_t <t_type const> get(tag_type const tag) const { return do_get <t_type const>(*this, tag); }
		template <tag_type t_tag>  constexpr get_value_return_type_t <tag_value_t <t_tag>> get() { return get <tag_value_t <t_tag>>(t_tag); }
		template <tag_type t_tag>  constexpr get_value_return_type_t <tag_value_t <t_tag> const> get() const { return get <tag_value_t <t_tag>>(t_tag); }

		// Get or insert (even on type mismatch).
		template <typename t_type> t_type &obtain(tag_type const tag);
		template <tag_type t_tag> tag_value_t <t_tag> &obtain() { return obtain <tag_value_t <t_tag>>(t_tag); }

		template <typename t_return, typename t_visitor>
		t_return visit_type(tag_type const type_index, t_visitor &&visitor);

		template <typename t_return, typename t_visitor>
		t_return visit(tag_rank const &tr, t_visitor &&visitor) const;

		bool compare_without_type_check(optional_field const &other) const;
		bool operator==(optional_field const &other) const;

		template <typename t_predicate>
		void erase_if(t_predicate &&predicate) { erase_if_(std::forward <t_predicate>(predicate), [](auto const, auto const){}); }

		template <typename t_predicate, typename t_callback>
		void erase_if(t_predicate &&predicate, t_callback &&erase_callback) { erase_if_(std::forward <t_predicate>(predicate), std::forward <t_callback>(erase_callback)); }
	};


	std::ostream &operator<<(std::ostream &os, optional_field const &of);
	std::ostream &operator<<(std::ostream &os, optional_field::tag_rank const tr); // For debugging.
	void output_optional_field_in_parsed_order(std::ostream &os, optional_field const &of, std::vector <std::size_t> &buffer);


	template <typename t_container_type>
	auto optional_field::prepare_for_adding(tag_type const tag_id) -> t_container_type &
	{
		// Precondition: libbio::sam::fields::optional_field (i.e. the parser) calls clear() before handling the optional fields of each record.
		constexpr auto const idx{tuples::first_index_of_v <value_tuple_type, t_container_type>};
		auto &dst(std::get <t_container_type>(m_values));
		m_tag_ranks.emplace_back(tag_id, idx, dst.size(), m_tag_ranks.size()); // Precondition used here for the last parameter.
		return dst;
	}


	template <typename t_type, typename t_value>
	void optional_field::add_value(tag_type const tag_id, t_value &&val)
	{
		auto &dst(prepare_for_adding <container_of_t <t_type>>(tag_id));
		dst.emplace_back(std::forward <t_value>(val));
	}


	std::string &optional_field::start_string(tag_type const tag_id)
	{
		auto &dst(prepare_for_adding <container_of_t <std::string>>(tag_id));
		return dst.emplace_back();
	}


	template <typename t_type>
	std::vector <t_type> &optional_field::start_array(tag_type const tag_id)
	{
		auto &dst(prepare_for_adding <array_vector <t_type>>(tag_id));
		return dst.emplace_back();
	}


	template <typename t_type, typename t_value>
	void optional_field::add_array_value(t_value const val)
	{
		std::get <array_vector <t_type>>(m_values).back().emplace_back(val);
	}


	template <typename t_type, typename t_optional_field>
	constexpr auto optional_field::find_rank(t_optional_field &&of, tag_type const tag) -> find_rank_return_type_t <t_optional_field>
	{
		auto const begin(of.m_tag_ranks.begin());
		auto const end(of.m_tag_ranks.end());
		auto const it(std::lower_bound(begin, end, tag, tag_rank_cmp{}));
		return it;
	}


	template <typename t_type, typename t_optional_field>
	constexpr auto optional_field::do_get(t_optional_field &&of, tag_rank_vector::const_iterator it, tag_type const tag) -> get_value_return_type_t <t_type>
	{
		typedef value_container_t <std::remove_const_t <t_type>> tuple_element_type;
		constexpr auto const idx{tuples::first_index_of_v <value_tuple_type, tuple_element_type>};
		auto const end(of.m_tag_ranks.cend());
		if (end == it) return std::unexpected{get_value_error::not_found};
		if (it->tag_id != tag) return std::unexpected{get_value_error::not_found};
		if (it->type_index != idx) return std::unexpected{get_value_error::type_mismatch};
		return std::ref(std::get <idx>(of.m_values)[it->rank]);	// Non-const of used here.
	}


	template <typename t_type, typename t_optional_field>
	constexpr auto optional_field::do_get(t_optional_field &&of, tag_type const tag) -> get_value_return_type_t <t_type>
	{
		auto const it(find_rank <t_type>(of, tag));
		return do_get <t_type>(of, it, tag);
	}


	template <typename t_type>
	t_type &optional_field::obtain(tag_type const tag)
	{
		typedef value_container_t <std::remove_const_t <t_type>> tuple_element_type;
		constexpr auto const idx{tuples::first_index_of_v <value_tuple_type, tuple_element_type>};
		static_assert(std::is_same_v <tuple_element_type, std::tuple_element_t <idx, value_tuple_type>>);

		auto const it(find_rank <t_type>(*this, tag));
		auto const end(m_tag_ranks.cend());
		auto &dst(std::get <idx>(m_values));

		if (end == it || it->tag_id != tag)
		{
			m_tag_ranks.emplace(it, tag, idx, dst.size());
			return dst.emplace_back();
		}

		// it->tag_id == tag.
		if (it->type_index == idx)
			return dst[it->rank];

		// it->type_index != idx.
		// Erase the old value and return a reference to a new one.
		erase_values_in_range(it, it + 1);
		for (auto it_(it + 1); end != it_; ++it_)
		{
			// Assign new ranks to the succeeding tags of the same type as the removed one.
			if (it->type_index == it_->type_index && it->rank < it_->rank)
				--it_->rank;
		}

		it->type_index = idx;
		it->rank = dst.size();
		return dst.emplace_back();
	}


	template <typename t_return, typename t_visitor>
	t_return optional_field::visit_type(tag_type const type_index, t_visitor &&visitor)
	{
		auto visitor_caller([this, &visitor]<typename t_idx_type> -> t_return {
			constexpr auto const idx{t_idx_type::value};
			auto &val(std::get <idx>(m_values));
			if (std::is_void_v <t_return>)
				visitor(val);
			else
				return visitor(val);
		});

		typedef detail::callback_table_builder <
			decltype(visitor_caller),
			tuples::index_constant_sequence_for_tuple <value_tuple_type>
		> callback_table_type;

		constexpr auto const &fns(callback_table_type::fns);
		if (std::is_void_v <t_return>)
			(visitor_caller.*fns[type_index])();
		else
			return (visitor_caller.*fns[type_index])();
	}


	template <typename t_return, typename t_visitor>
	t_return optional_field::visit(tag_rank const &tr, t_visitor &&visitor) const
	{
		auto visitor_caller([this, rank = tr.rank, &visitor]<typename t_idx_type> -> t_return {
			constexpr auto const idx{t_idx_type::value};
			constexpr auto const type_code{type_codes[idx]};
			// Pass the type code as a template parameter to the visitor.
			auto &vec(std::get <idx>(m_values));
			libbio_assert_lt(rank, vec.size());
			auto &val(vec[rank]);
			if (std::is_void_v <t_return>)
				visitor.template operator()<idx, type_code>(val);
			else
				return visitor.template operator()<idx, type_code>(val);
		});

		typedef detail::callback_table_builder <
			decltype(visitor_caller),
			tuples::index_constant_sequence_for_tuple <value_tuple_type>
		> callback_table_type;

		constexpr auto const &fns(callback_table_type::fns);
		if (! (tr.type_index < fns.size()))
			throw std::invalid_argument("Invalid type index");

		if (std::is_void_v <t_return>)
			(visitor_caller.*fns[tr.type_index])();
		else
			return (visitor_caller.*fns[tr.type_index])();
	}


	template <typename t_predicate, typename t_erase_callback>
	void optional_field::erase_if_(t_predicate &&predicate, t_erase_callback &&callback)
	{
		// Partition the ranks and the values in such a way that the order of the preserved tags is maintained.
		// This way new ranks can be assigned to the maintained items quite easily.

		// Find the range [it, m_tag_ranks.end()) of items to be removed and sort.
		auto const rank_end(m_tag_ranks.end());
		auto rank_it(libbio::stable_partition_left(m_tag_ranks.begin(), rank_end, std::not_fn(predicate)));
		auto const rank_it_(rank_it);
		std::sort(rank_it, rank_end, [](tag_rank const &lhs, tag_rank const &rhs){
			return lhs.type_and_rank_to_tuple() < rhs.type_and_rank_to_tuple();
		});

		// Process each vector or array_vector.
		// Find runs of the same type_index and use erase_values_in_range().
		while (rank_it != rank_end)
		{
			auto const rank_mid(std::partition_point(rank_it, rank_end, [type_index = rank_it->type_index](tag_rank const &tr){
				return tr.type_index == type_index;
			}));
			// Calls lb::remove_at_indices if the vector contains scalar values and lb::stable_partition_left_at_indices in case of vector values, clearing the vectors on the right.
			erase_values_in_range(rank_it, rank_mid);
			rank_it = rank_mid;
		}

		// Erase the removed ranks.
		callback(tag_rank_vector::const_iterator(rank_it_), m_tag_ranks.cend());
		m_tag_ranks.erase(rank_it_, rank_end);

		// Assign new ranks to the remaining tags.
		{
			std::array <tag_count_type, std::tuple_size_v <value_tuple_type>> new_ranks{};
			for (auto &tr : m_tag_ranks)
				tr.rank = new_ranks[tr.type_index]++;
		}
	}
}

#endif

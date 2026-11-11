/*
 * Copyright (c) 2026 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_FUNCTION_INPUT_ITERATOR_HH
#define LIBBIO_FUNCTION_INPUT_ITERATOR_HH

#include <cstddef>		// std::ptrdiff_t
#include <iterator>
#include <memory>


namespace libbio::iterators::detail {

	template <typename t_fn>
	struct input_function_traits
	{
		typedef t_fn *stored_function_type;
		constexpr static inline bool is_shared_pointer{};
	};

	template <typename t_value>
	struct input_function_traits <std::shared_ptr <t_value>>
	{
		typedef std::shared_ptr <t_value> stored_function_type;
		constexpr static inline bool is_shared_pointer{true};
	};
}


namespace libbio {

	// A simple non-owning function input iterator that satisfies LegacyInputIterator.
	template <typename t_value, typename t_fn>
	class function_input_iterator
	{
	public:
		typedef iterators::detail::input_function_traits <t_fn> input_function_traits;
		typedef typename input_function_traits::stored_function_type stored_function_type;

		typedef std::input_iterator_tag iterator_category;
		typedef std::ptrdiff_t difference_type;
		typedef t_value const value_type;
		typedef t_value const &reference;
		typedef t_value const *pointer;

	private:
		stored_function_type	m_fn{};
		t_value					m_current_value{}; // value_type is const.
		bool					m_has_value{};

	private:
		void advance() { m_has_value = (*m_fn)(m_current_value); }

	public:
		function_input_iterator() = default;

		explicit function_input_iterator(t_fn &fn):
			m_fn{fn}
		{
			advance();
		}

		explicit function_input_iterator(t_fn &&fn)
		requires(input_function_traits::is_shared_pointer):
			m_fn{std::forward <t_fn>(fn)}
		{
			advance();
		}

		reference operator*() const { return m_current_value; }
		function_input_iterator &operator++() { advance(); return *this; }
		bool operator==(function_input_iterator const &other) const;
	};


	template <typename t_context, typename t_value>
	bool function_input_iterator <t_context, t_value>::operator==(
		function_input_iterator const &other
	) const
	{
		return (!m_fn || !other.m_fn) && m_has_value == other.m_has_value;
	}
}

#endif

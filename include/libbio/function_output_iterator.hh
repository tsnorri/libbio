/*
 * Copyright (c) 2022-2026 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_FUNCTION_OUTPUT_ITERATOR_HH
#define LIBBIO_FUNCTION_OUTPUT_ITERATOR_HH

#include <cstddef>		// std::ptrdiff_t
#include <type_traits>


namespace libbio::iterators::detail {

	template <typename t_fn>
	class call_proxy
	{
	private:
		t_fn *m_fn{};

	public:
		explicit call_proxy(t_fn &fn): m_fn{&fn} {}

		call_proxy() = default;
		call_proxy(call_proxy const &) = default;
		call_proxy(call_proxy &&) = default;
		call_proxy &operator=(call_proxy const &) = delete;
		call_proxy &operator=(call_proxy &&) = delete;

		template <typename t_value>
		requires (!std::is_same_v <std::remove_cvref_t <t_value>, call_proxy>)
		call_proxy const &operator=(t_value &&value) const { (*m_fn)(value); return *this; }
	};
}


namespace libbio {

	// A simple non-owning function output iterator that satisfies both LegacyOutputIterator
	// and std::output_iterator. Iterators are often copied so we store a pointer to the
	// function to be called.
	template <typename t_fn>
	class function_output_iterator
	{
	public:
		typedef std::ptrdiff_t difference_type; // Needed for std::output_iterator.
		typedef void value_type;
		typedef void reference;
		typedef void pointer;

	private:
		typedef iterators::detail::call_proxy <t_fn> call_proxy_type;

	private:
		t_fn	*m_fn{};

	public:
		explicit function_output_iterator(t_fn &fn):
			m_fn{&fn}
		{
		}

		call_proxy_type operator*() { return call_proxy_type{*m_fn}; }

		function_output_iterator &operator++() { return *this; }	// Return *this.
		function_output_iterator &operator++(int) { return *this; }	// Return *this.
	};
}

#endif

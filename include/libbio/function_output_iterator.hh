/*
 * Copyright (c) 2022-2026 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_FUNCTION_OUTPUT_ITERATOR_HH
#define LIBBIO_FUNCTION_OUTPUT_ITERATOR_HH

#include <cstddef>		// std::ptrdiff_t
#include <stdexcept>	// std::runtime_error
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


	class function_output_iterator_base
	{
	public:
		typedef std::ptrdiff_t difference_type; // Needed for std::output_iterator.
		typedef void value_type;
		typedef void reference;
		typedef void pointer;

	public:
		// ranges::semiregular <T> (in range-v3/include/concepts/concepts.hpp) requires copyable and default_constructible.
		// m_fn is needed for the iterator to work, though, so throw in case the default constructor is somehow called.
		function_output_iterator_base()
		{
			throw std::runtime_error("function_output_iterator’s default constructor should not be called.");
		}

		function_output_iterator_base &operator++() { return *this; }		// Return *this.
		function_output_iterator_base &operator++(int) { return *this; }	// Return *this.
	};
}


namespace libbio {

	// A simple non-owning function output iterator that satisfies both LegacyOutputIterator
	// and std::output_iterator.
	template <typename t_fn>
	class function_output_iterator : public iterators::detail::function_output_iterator_base
	{
	private:
		typedef iterators::detail::call_proxy <t_fn> call_proxy_type;

	private:
		t_fn	*m_fn{};

	public:
		using iterators::detail::function_output_iterator_base::function_output_iterator_base;

		function_output_iterator(t_fn &fn):
			m_fn{&fn}
		{
		}

		call_proxy_type operator*() { return call_proxy_type{*m_fn}; }
	};


	// Contains a context instead of a function pointer.
	template <typename t_context>
	class function_output_context_iterator : public iterators::detail::function_output_iterator_base
	{
	public:
		typedef t_context context_type;

	private:
		typedef iterators::detail::call_proxy <t_context> call_proxy_type;

	private:
		t_context	m_ctx{};

	public:
		using iterators::detail::function_output_iterator_base::function_output_iterator_base;

		function_output_context_iterator() = default;

		function_output_context_iterator(context_type &&ctx):
			m_ctx{std::forward <t_context>(ctx)}
		{
		}

		context_type &context() { return m_ctx; }
		context_type const &context() const { return m_ctx; }

		call_proxy_type operator*() { return call_proxy_type{m_ctx}; }
	};

}

#endif

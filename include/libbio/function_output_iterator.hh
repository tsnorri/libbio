/*
 * Copyright (c) 2022-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_FUNCTION_OUTPUT_ITERATOR_HH
#define LIBBIO_FUNCTION_OUTPUT_ITERATOR_HH

#include <cstddef>		// std::ptrdiff_t
#include <stdexcept>	// std::runtime_error
#include <utility>		// std::forward


namespace libbio {

	// A simple non-owning function output iterator compatible with range-v3.
	template <typename t_fn>
	class function_output_iterator
	{
	public:
		typedef std::ptrdiff_t difference_type; // Needed for range-v3.

	protected:
		t_fn	*m_fn{};

	public:
		// ranges::semiregular<T> (in range-v3/include/concepts/concepts.hpp) requires copyable and default_constructible.
		// m_dst is needed for the iterator to work, though, so throw in case the default constructor is somehow called.
		function_output_iterator()
		{
			throw std::runtime_error("function_output_iterator’s default constructor should not be called.");
		}

		function_output_iterator(t_fn &fn):
			m_fn(&fn)
		{
		}

		function_output_iterator &operator++() { return *this; }			// Return *this.
		function_output_iterator &operator++(int) { return *this; }			// Return *this.
		function_output_iterator &operator*() { return *this; }				// Return *this.
		function_output_iterator const &operator*() const { return *this; }	// Return *this.

		template <typename t_arg>
		function_output_iterator &operator=(t_arg &&arg) { (*m_fn)(std::forward <t_arg>(arg)); return *this; }
	};
}

#endif

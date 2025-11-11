/*
 * Copyright (c) 2025 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_UTILITY_VARIABLE_GUARD_HH
#define LIBBIO_UTILITY_VARIABLE_GUARD_HH

namespace libbio {

	template <typename t_type>
	struct variable_guard
	{
		t_type * &variable;

		variable_guard(t_type * &variable_, t_type &value_):
			variable{variable_}
		{
			variable = &value_;
		}

		~variable_guard() { variable = nullptr; }

		variable_guard(variable_guard const &) = delete;
		variable_guard(variable_guard &&) = delete;
		variable_guard &operator=(variable_guard const &) = delete;
		variable_guard &operator=(variable_guard &&) = delete;
	};
}

#endif

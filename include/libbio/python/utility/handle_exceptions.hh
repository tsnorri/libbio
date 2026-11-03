/*
 * Copyright (c) 2025-2026 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_PYTHON_UTILITY_HANDLE_EXCEPTIONS_HH
#define LIBBIO_PYTHON_UTILITY_HANDLE_EXCEPTIONS_HH

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <libbio/assert.hh>
#include <exception>
#include <type_traits>


namespace libbio::python {

	struct exception_already_handled {};


	template <typename t_fn, typename t_return = std::invoke_result_t <t_fn>>
	auto handle_exceptions(t_fn &&fn, t_return const error_retval = t_return{}) -> t_return
	{
		try
		{
			return fn();
		}
		catch (exception_already_handled)
		{
			// Python error already set.
		}
		catch (libbio::assertion_failure_exception const &exc)
		{
			PyErr_SetString(PyExc_AssertionError, exc.what());
		}
		catch (std::exception const &exc)
		{
			PyErr_SetString(PyExc_RuntimeError, exc.what());
		}
		catch (...)
		{
			PyErr_SetString(PyExc_RuntimeError, "Unknown C++ error");
		}

		return error_retval;
	}
}

#endif

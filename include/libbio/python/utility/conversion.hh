/*
 * Copyright (c) 2025-2026 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_PYTHON_UTILITY_CONVERSION_HH
#define LIBBIO_PYTHON_UTILITY_CONVERSION_HH

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <cstddef>
#include <cstdint>
#include <libbio/python/utility/handle_exceptions.hh>


namespace libbio::python {

	inline PyObject *new_py_long(long vv) { return PyLong_FromLong(vv); }
	inline PyObject *new_py_long(unsigned long vv) { return PyLong_FromUnsignedLong(vv); }
	inline PyObject *new_py_long(long long vv) { return PyLong_FromLongLong(vv); }
	inline PyObject *new_py_long(unsigned long long vv) { return PyLong_FromUnsignedLongLong(vv); }


	// Return true on success.
	inline bool py_long_value(PyObject *obj, std::int32_t &value) { return 0 == PyLong_AsInt32(obj, &value); }
	inline bool py_long_value(PyObject *obj, std::int64_t &value) { return 0 == PyLong_AsInt64(obj, &value); }
	inline bool py_long_value(PyObject *obj, std::uint32_t &value) { return 0 == PyLong_AsUInt32(obj, &value); }
	inline bool py_long_value(PyObject *obj, std::uint64_t &value) { return 0 == PyLong_AsUInt64(obj, &value); }


	template <typename t_type>
	t_type check_and_get_long_value(PyObject *obj, std::size_t arg_idx)
	{
		if (! PyLong_Check(obj))
		{
			PyErr_Format(PyExc_TypeError, "Expected a long as argument %zu", arg_idx);
			throw exception_already_handled{};
		}

		t_type value{};
		if (! py_long_value(obj, value))
			throw exception_already_handled{};

		return value;
	}
}

#endif

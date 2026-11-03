/*
 * Copyright (c) 2026 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_PYTHON_WRAPPER_DISJOINT_SETS_HH
#define LIBBIO_PYTHON_WRAPPER_DISJOINT_SETS_HH

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <libbio/disjoint_sets.hh>
#include <libbio/python/utility/conversion.hh>
#include <libbio/python/utility/handle_exceptions.hh>


namespace libbio::python {

	template <typename t_vector>
	struct disjoint_sets
	{
		PyObject_HEAD;

		typedef libbio::disjoint_sets <t_vector> disjoint_sets_type;
		typedef typename disjoint_sets_type::value_type value_type;

		union
		{
			char dummy;
			disjoint_sets_type disjoint_sets_;
		};

		disjoint_sets() = delete;
		~disjoint_sets() {}

		// The types of static member functions (unlike those of non-static
		// member functions) are the same as those of ordinary functions.
		static int init(disjoint_sets *self, PyObject *args, PyObject **kwargs) noexcept;
		static void dealloc(disjoint_sets *self) noexcept;

		static PyObject *is_singleton(disjoint_sets *self, PyObject *value) noexcept;
		static PyObject *find(disjoint_sets *self, PyObject *value) noexcept;
		static PyObject *set_union(disjoint_sets *self, PyObject * const *args, Py_ssize_t const arg_count) noexcept;

		static PyTypeObject make_type_object(char const *name) noexcept;
	};


	template <typename t_vector>
	PyTypeObject disjoint_sets <t_vector>::make_type_object(char const *name) noexcept
	{
		static PyMethodDef methods[]{
			{"is_singleton", reinterpret_cast <PyCFunction>(is_singleton), METH_O, "Check if the given set is a singleton"},
			{"find", reinterpret_cast <PyCFunction>(find), METH_O, "Find the representative for the given set"},
			{"set_union", reinterpret_cast <PyCFunction>(set_union), METH_FASTCALL, "Join the given two sets"},
			{}
		};

		return {
			.ob_base = PyVarObject_HEAD_INIT(nullptr, 0)
			.tp_name = name,
			.tp_basicsize = sizeof(disjoint_sets),
			.tp_itemsize = 0,
			.tp_dealloc = reinterpret_cast <destructor>(dealloc),
			.tp_flags = Py_TPFLAGS_DEFAULT,
			.tp_doc = PyDoc_STR("Disjoint sets"),
			.tp_methods = methods,
			.tp_init = reinterpret_cast <initproc>(init),
			.tp_new = PyType_GenericNew
		};
	}


	template <typename t_vector>
	int disjoint_sets <t_vector>::init(disjoint_sets *self, PyObject *args, PyObject **kwargs) noexcept
	{
		return handle_exceptions([&]{
			new (&self->disjoint_sets_) disjoint_sets_type{};
			return 0;
		}, -1);
	}


	template <typename t_vector>
	void disjoint_sets <t_vector>::dealloc(disjoint_sets *self) noexcept
	{
		self->disjoint_sets_.~disjoint_sets_type();
		Py_TYPE(self)->tp_free(self);
	}


	template <typename t_vector>
	PyObject *disjoint_sets <t_vector>::is_singleton(disjoint_sets *self, PyObject *value) noexcept
	{
		return handle_exceptions([&] -> PyObject* {
			auto const value_{check_and_get_long_value <value_type>(value, 1)};
			return (self->disjoint_sets_.is_singleton(value_) ? Py_True : Py_False);
		});
	}


	template <typename t_vector>
	PyObject *disjoint_sets <t_vector>::find(disjoint_sets *self, PyObject *value) noexcept
	{
		return handle_exceptions([&] -> PyObject* {
			auto const value_{check_and_get_long_value <value_type>(value, 1)};
			auto const res{self->disjoint_sets_.find(value_)};
			return new_py_long(res);
		});
	}


	template <typename t_vector>
	PyObject *disjoint_sets <t_vector>::set_union(
		disjoint_sets *self,
		PyObject * const *args,
		Py_ssize_t const arg_count
	) noexcept
	{
		return handle_exceptions([&] -> PyObject* {
			if (2 != arg_count)
			{
				PyErr_Format(PyExc_TypeError, "Expected 2 arguments, got %zd", arg_count);
				return nullptr;
			}

			auto const lhs{check_and_get_long_value <value_type>(args[0], 1)};
			auto const rhs{check_and_get_long_value <value_type>(args[1], 2)};
			self->disjoint_sets_.set_union(lhs, rhs);
			Py_RETURN_NONE;
		});
	}
}

#endif

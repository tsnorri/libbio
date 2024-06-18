/*
 * Copyright (c) 2017-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_ASSERT_HH
#define LIBBIO_ASSERT_HH

#include <boost/stacktrace.hpp>
#include <boost/exception/all.hpp>
#include <iostream>
#include <libbio/buffer.hh>
#include <libbio/utility/is_equal.hh>
#include <libbio/utility/is_lt.hh>
#include <libbio/utility/is_lte.hh>
#include <libbio/utility/misc.hh>
#include <sstream>
#include <stdexcept>


#define libbio_stringify(X) (#X)

#define libbio_assert_test(TEST, MESSAGE)					do { ::libbio::detail::assert_test(bool(TEST), __FILE__, __LINE__, MESSAGE); } while (false)
#define libbio_assert_test_bin(LHS, RHS, TEST, MESSAGE)		do { ::libbio::detail::assert_test_bin <TEST>(LHS, RHS, __FILE__, __LINE__, MESSAGE); } while (false)
// Requires formatted output.
#define libbio_assert_test_bin_(LHS, RHS, TEST, MESSAGE)	do { ::libbio::detail::assert_test_bin <TEST, true>(LHS, RHS, __FILE__, __LINE__, MESSAGE); } while (false)
#define libbio_assert_test_msg(TEST, ...)					do { ::libbio::detail::assert_test(bool(TEST), __FILE__, __LINE__, __VA_ARGS__); } while (false)
#define libbio_assert_test_rel_msg(TEST, REL_EXPR, ...)		do { ::libbio::detail::assert_test(bool(TEST), __FILE__, __LINE__, __VA_ARGS__, ": ", (REL_EXPR), '.'); } while (false)

// FIXME: Make this work when immediately evaluated.
#define libbio_fail(...) do { \
		::libbio::detail::assertion_failure(__FILE__, __LINE__, __VA_ARGS__); \
	} while (false)

#define libbio_always_assert(X)					libbio_assert_test((X),											#X)
#define libbio_always_assert_lt(X, Y)			libbio_assert_test_bin((X), (Y), ::libbio::is_lt_,				libbio_stringify(X <  #Y))
#define libbio_always_assert_lte(X, Y)			libbio_assert_test_bin((X), (Y), ::libbio::is_lte_,				libbio_stringify(X <= #Y))
#define libbio_always_assert_gt(X, Y)			libbio_assert_test_bin((X), (Y), ::libbio::is_not_lte_,			libbio_stringify(X >  #Y))
#define libbio_always_assert_gte(X, Y)			libbio_assert_test_bin((X), (Y), ::libbio::is_not_lt_,			libbio_stringify(X >= #Y))
#define libbio_always_assert_eq(X, Y)			libbio_assert_test_bin((X), (Y), ::libbio::is_equal_,			libbio_stringify(X == #Y))
#define libbio_always_assert_neq(X, Y)			libbio_assert_test_bin((X), (Y), ::libbio::is_not_equal_,		libbio_stringify(X != #Y))

// Variants that require X and Y to be printable using operator <<.
#define libbio_always_assert_lt_(X, Y)			libbio_assert_test_bin_((X), (Y), ::libbio::is_lt_,				libbio_stringify(X <  #Y))
#define libbio_always_assert_lte_(X, Y)			libbio_assert_test_bin_((X), (Y), ::libbio::is_lte_,			libbio_stringify(X <= #Y))
#define libbio_always_assert_gt_(X, Y)			libbio_assert_test_bin_((X), (Y), ::libbio::is_not_lte_,		libbio_stringify(X >  #Y))
#define libbio_always_assert_gte_(X, Y)			libbio_assert_test_bin_((X), (Y), ::libbio::is_not_lt_,			libbio_stringify(X >= #Y))
#define libbio_always_assert_eq_(X, Y)			libbio_assert_test_bin_((X), (Y), ::libbio::is_equal_,			libbio_stringify(X == #Y))
#define libbio_always_assert_neq_(X, Y)			libbio_assert_test_bin_((X), (Y), ::libbio::is_not_equal_,		libbio_stringify(X != #Y))

#define libbio_always_assert_msg(X, ...)		libbio_assert_test_msg((X),																	__VA_ARGS__)
#define libbio_always_assert_lt_msg(X, Y, ...)	libbio_assert_test_rel_msg(::libbio::is_lt((X), (Y)),			libbio_stringify(X < Y),	__VA_ARGS__)
#define libbio_always_assert_lte_msg(X, Y, ...)	libbio_assert_test_rel_msg(::libbio::is_lte((X), (Y)),			libbio_stringify(X <= Y),	__VA_ARGS__)
#define libbio_always_assert_gt_msg(X, Y, ...)	libbio_assert_test_rel_msg(::libbio::is_lt((Y), (X)),			libbio_stringify(X > Y),	__VA_ARGS__)
#define libbio_always_assert_gte_msg(X, Y, ...)	libbio_assert_test_rel_msg(::libbio::is_lte((Y), (X)),			libbio_stringify(X >= Y),	__VA_ARGS__)
#define libbio_always_assert_eq_msg(X, Y, ...)	libbio_assert_test_rel_msg(::libbio::is_equal((X), (Y)),		libbio_stringify(X == Y),	__VA_ARGS__)
#define libbio_always_assert_neq_msg(X, Y, ...)	libbio_assert_test_rel_msg(::libbio::is_not_equal((X), (Y)),	libbio_stringify(X != Y),	__VA_ARGS__)

#ifdef LIBBIO_NDEBUG
#	define libbio_assert(X)
#	define libbio_assert_lt(X, Y)
#	define libbio_assert_lte(X, Y)
#	define libbio_assert_gt(X, Y)
#	define libbio_assert_gte(X, Y)
#	define libbio_assert_eq(X, Y)
#	define libbio_assert_neq(X, Y)

#	define libbio_assert_(X)
#	define libbio_assert_lt_(X, Y)
#	define libbio_assert_lte_(X, Y)
#	define libbio_assert_gt_(X, Y)
#	define libbio_assert_gte_(X, Y)
#	define libbio_assert_eq_(X, Y)
#	define libbio_assert_neq_(X, Y)

#	define libbio_do_and_assert_eq(X, Y)	do { (X); } while (false) // FIXME: try to determine what this is supposed to do.

#	define libbio_assert_msg(X, ...)
#	define libbio_assert_lt_msg(X, Y, ...)
#	define libbio_assert_lte_msg(X, Y, ...)
#	define libbio_assert_gt_msg(X, Y, ...)
#	define libbio_assert_gte_msg(X, Y, ...)
#	define libbio_assert_eq_msg(X, Y, ...)
#	define libbio_assert_neq_msg(X, Y, ...)
#else
#	define libbio_assert(X)					libbio_always_assert((X))
#	define libbio_assert_lt(X, Y)			libbio_always_assert_lt((X), (Y))
#	define libbio_assert_lte(X, Y)			libbio_always_assert_lte((X), (Y))
#	define libbio_assert_gt(X, Y)			libbio_always_assert_gt((X), (Y))
#	define libbio_assert_gte(X, Y)			libbio_always_assert_gte((X), (Y))
#	define libbio_assert_eq(X, Y)			libbio_always_assert_eq((X), (Y))
#	define libbio_assert_neq(X, Y)			libbio_always_assert_neq((X), (Y))

// Variants that require X and Y to be printable using operator <<.
#	define libbio_assert_(X)				libbio_always_assert_((X))
#	define libbio_assert_lt_(X, Y)			libbio_always_assert_lt_((X), (Y))
#	define libbio_assert_lte_(X, Y)			libbio_always_assert_lte_((X), (Y))
#	define libbio_assert_gt_(X, Y)			libbio_always_assert_gt_((X), (Y))
#	define libbio_assert_gte_(X, Y)			libbio_always_assert_gte_((X), (Y))
#	define libbio_assert_eq_(X, Y)			libbio_always_assert_eq_((X), (Y))
#	define libbio_assert_neq_(X, Y)			libbio_always_assert_neq_((X), (Y))

#	define libbio_do_and_assert_eq(X, Y)	libbio_always_assert_eq((X), (Y))
		
#	define libbio_assert_msg(X, ...)		libbio_always_assert_msg((X), __VA_ARGS__)
#	define libbio_assert_lt_msg(X, Y, ...)	libbio_always_assert_lt_msg((X), (Y), __VA_ARGS__)
#	define libbio_assert_lte_msg(X, Y, ...)	libbio_always_assert_lte_msg((X), (Y), __VA_ARGS__)
#	define libbio_assert_gt_msg(X, Y, ...)	libbio_always_assert_gt_msg((X), (Y), __VA_ARGS__)
#	define libbio_assert_gte_msg(X, Y, ...)	libbio_always_assert_gte_msg((X), (Y), __VA_ARGS__)
#	define libbio_assert_eq_msg(X, Y, ...)	libbio_always_assert_eq_msg((X), (Y), __VA_ARGS__)
#	define libbio_assert_neq_msg(X, Y, ...)	libbio_always_assert_neq_msg((X), (Y), __VA_ARGS__)
#endif


namespace libbio { namespace detail {

	template <typename t_type>
	struct has_formatted_output_function_helper
	{
		template <typename T>
		static decltype(std::declval <std::ostream>() << std::declval <T>()) test(T const *);
		
		template <typename>
		static std::false_type test(...);
		
		constexpr static inline bool value{std::is_same_v <std::ostream &, decltype(test <t_type>(nullptr))>};
	};

	template <typename t_type>
	constexpr inline bool has_formatted_output_function_v = has_formatted_output_function_helper <t_type>::value;
	
	
	static_assert(has_formatted_output_function_v <long const>);
	static_assert(has_formatted_output_function_v <long>);

	
	// Copying a standard library class derived from std::exception
	// is not permitted to throw exceptions, so try to avoid it here, too.
	struct assertion_failure_cause
	{
		typedef buffer <char>	buffer_type;
		
		std::string		reason;
		std::string		file;
		buffer_type		what;
		long			line{};

		assertion_failure_cause() = default;

		assertion_failure_cause(char const *file_, long line_):
			file(file_),
			what(
				buffer_type::buffer_with_allocated_buffer(
					copy_format_cstr(boost::format("%s:%d") % file % line_)
				)
			),
			line(line_)
		{
		}

		assertion_failure_cause(char const *file_, long line_, std::string &&reason_):
			reason(std::move(reason_)),
			file(file_),
			what(
				buffer_type::buffer_with_allocated_buffer(
					copy_format_cstr(boost::format("%s:%d: %s") % file % line_ % reason)
				)
			),
			line(line_)
		{
		}
	};


	// Fwd.
	[[noreturn]] constexpr inline void assertion_failure(char const *file, long const line);
	[[noreturn]] constexpr inline void assertion_failure(char const *file, long const line, char const *assertion);
	[[noreturn]] inline void assertion_failure(char const *file, long const line, std::string const &assertion);


	// Concatenate the arguments and call assertion_failure.
	template <typename t_arg>
	[[noreturn]] void assertion_failure__(char const *file, long const line, std::stringstream &stream, t_arg const &arg)
	{
		stream << arg;
		auto const &string(stream.str()); // Keep the result of str() here to prevent a dangling pointer.
		assertion_failure(file, line, string.c_str()); // Copies the result of c_str().
	}
	

	// Concatenate the arguments and call assertion_failure.
	template <typename t_arg, typename ... t_rest>
	[[noreturn]] void assertion_failure__(char const *file, long const line, std::stringstream &stream, t_arg const &first, t_rest && ... rest)
	{
		stream << first;
		assertion_failure__(file, line, stream, rest...);
	}
	
	
	// Concatenate the arguments and call assertion_failure.
	template <typename ... t_args>
	[[noreturn]] void assertion_failure_(char const *file, long const line, t_args && ... args)
	{
		// For some reason if std::stringstream is instantiated in the non-consteval branch of if consteval in a constexpr function, Clang++14 rejects the code.
		// FIXME: use std::format.
		std::stringstream stream;
		assertion_failure__(file, line, stream, std::forward <t_args>(args)...);
	}
	
	
	template <typename ... t_args>
	[[noreturn]] constexpr void assertion_failure(char const *file, long const line, t_args && ... args)
	{
		if consteval
		{
			assertion_failure(file, line, "(Formatted output for consteval assertion failure not implemented.)");
		}
		else
		{
			assertion_failure_(file, line, std::forward <t_args>(args)...);
		}
	}


	template <bool t_requires_formatted_output_fn, typename t_lhs, typename t_rhs>
	[[noreturn]] void assertion_failure_(char const *file, long const line, t_lhs const &lhs, t_rhs const &rhs, char const *message)
	requires (!t_requires_formatted_output_fn || (has_formatted_output_function_v <t_lhs> && has_formatted_output_function_v <t_rhs>))
	{
		// Clang++ 14 is not happy about the use of std::stringstream in a constexpr function even if in the false branch
		// of if consteval, so we build the message here instead.
		if constexpr (has_formatted_output_function_v <t_lhs> && has_formatted_output_function_v <t_rhs>)
		{
			// FIXME: use std::format.
			std::stringstream stream;
			stream << message << " (lhs: " << lhs << ", rhs: " << rhs << ')';
			assertion_failure(file, line, stream.str());
		}
		else
		{
			assertion_failure(file, line, message);
		}
	}


	template <bool t_requires_formatted_output_fn = false, typename t_lhs, typename t_rhs>
	[[noreturn]] constexpr void assertion_failure(char const *file, long const line, t_lhs const &lhs, t_rhs const &rhs, char const *message)
	{
		if consteval
		{
			assertion_failure(file, line, message);
		}
		else
		{
			assertion_failure_ <t_requires_formatted_output_fn>(file, line, lhs, rhs, message);
		}
	}


	template <typename t_line, typename ... t_args>
	constexpr inline void assert_test(bool const test, char const *file, t_line const line, t_args && ... args)
	{
		if (!test)
			assertion_failure(file, line, std::forward <t_args>(args)...);
	}


	template <typename t_test, bool t_requires_formatted_output_fn = false, typename t_lhs, typename t_rhs, typename t_line>
	constexpr inline void assert_test_bin(t_lhs &&lhs, t_rhs &&rhs, char const *file, t_line const line, char const *message)
	{
		t_test test;
		if (!test(lhs, rhs))
			assertion_failure <t_requires_formatted_output_fn>(file, line, lhs, rhs, message);
	}
}}


namespace libbio {

	typedef boost::error_info <struct tag_stacktrace, boost::stacktrace::stacktrace> traced;

	
	class assertion_failure_exception : public std::exception
	{
	protected:
		std::shared_ptr <detail::assertion_failure_cause>	m_cause;

	public:
		assertion_failure_exception() = default;

		assertion_failure_exception(char const *file, long line, std::string &&reason):
			m_cause(new detail::assertion_failure_cause(file, line, std::move(reason)))
		{
		}

		assertion_failure_exception(char const *file, long line):
			m_cause(new detail::assertion_failure_cause(file, line))
		{
		}

		virtual char const *what() const noexcept override { return m_cause->what.get(); }
		std::string const &file() const noexcept { return m_cause->file; }
		std::string const &reason() const noexcept { return m_cause->reason; }
		long line() const noexcept { return m_cause->line; }
	};
}


namespace libbio::detail {

	template <typename t_exception, typename ... t_args>
	[[noreturn]] constexpr void throw_with_trace(t_args ... args)
	{
		throw boost::enable_error_info(t_exception(args...)) << traced(boost::stacktrace::stacktrace());
	}

	
	[[noreturn]] constexpr inline void assertion_failure(char const *file, long const line)
	{
		throw_with_trace <assertion_failure_exception>(file, line);
	}


	[[noreturn]] constexpr inline void assertion_failure(char const *file, long const line, char const *assertion)
	{
		throw_with_trace <assertion_failure_exception> (file, line, assertion);
	}


	[[noreturn]] inline void assertion_failure(char const *file, long const line, std::string const &assertion)
	{
		assertion_failure(file, line, assertion.c_str());
	}
}

#endif

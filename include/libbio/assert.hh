/*
 * Copyright (c) 2017-2025 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_ASSERT_HH
#define LIBBIO_ASSERT_HH

#include <exception>
#include <format>
#include <libbio/utility/is_equal.hh>
#include <libbio/utility/is_lt.hh>
#include <libbio/utility/is_lte.hh>
#include <memory>
#include <ostream>
#include <string>
#include <utility>

#ifdef LIBBIO_USE_BOOST_STACKTRACE
#	include <boost/stacktrace.hpp>
#	include <boost/exception/all.hpp>
#endif


#define libbio_stringify(X) (#X)
#define libbio_append_to_first_str_literal(SUFFIX, STR, ...) (STR SUFFIX), __VA_ARGS__

#define libbio_assert_test(TEST, MESSAGE)					do { ::libbio::detail::assert_test(bool(TEST), __FILE__, __LINE__, MESSAGE); } while (false)
#define libbio_assert_test_bin(LHS, RHS, TEST, MESSAGE)		do { ::libbio::detail::assert_test_bin <TEST>(LHS, RHS, __FILE__, __LINE__, MESSAGE); } while (false)
// Requires formatted output.
#define libbio_assert_test_bin_(LHS, RHS, TEST, MESSAGE)	do { ::libbio::detail::assert_test_bin <TEST, true>(LHS, RHS, __FILE__, __LINE__, MESSAGE); } while (false)
// Use std::format.
#define libbio_assert_test_msg(TEST, ...)					do { ::libbio::detail::assert_test(bool(TEST), __FILE__, __LINE__, __VA_ARGS__); } while (false)
#define libbio_assert_test_rel_msg(TEST, REL_EXPR, ...)		do { ::libbio::detail::assert_test(bool(TEST), __FILE__, __LINE__, libbio_append_to_first_str_literal(": {}.", __VA_ARGS__), (REL_EXPR)); } while (false)

// FIXME: Make this work when immediately evaluated.
#define libbio_fail(...) do { \
		::libbio::detail::assertion_failure_(__FILE__, __LINE__, __VA_ARGS__); \
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
#define libbio_always_assert_neq_msg(X, Y, ...)	libbio_assert_test_rel_msg((!::libbio::is_equal((X), (Y))),		libbio_stringify(X != Y),	__VA_ARGS__)

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

#	define libbio_do_and_assert_eq(X, Y)	do { (X); } while (false)

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


namespace libbio::detail {

	template <typename t_type>
	concept has_formatted_output_function_v = requires(t_type tt, std::ostream &os)
	{
		os << tt;
	};


	static_assert(has_formatted_output_function_v <long const>);
	static_assert(has_formatted_output_function_v <long>);


	// Copying a standard library class derived from std::exception
	// is not permitted to throw exceptions, so try to avoid it here, too.
	struct assertion_failure_cause
	{
		std::string		reason;
		std::string		file;
		std::string		what;
		long			line{};

		assertion_failure_cause() = default;

		assertion_failure_cause(char const *file_, long line_):
			file{file_},
			what{std::format("{}:{}", file, line_)},
			line{line_}
		{
		}

		assertion_failure_cause(char const *file_, long line_, std::string const &reason_):
			reason{reason_},
			file{file_},
			what{std::format("{}:{}: {}", file, line_, reason)},
			line{line_}
		{
		}

		assertion_failure_cause(char const *file_, long line_, std::string &&reason_):
			reason{std::move(reason_)},
			file{file_},
			what{std::format("{}:{}: {}", file, line_, reason)},
			line{line_}
		{
		}
	};


	// Fwd.
	[[noreturn]] constexpr inline void assertion_failure_(char const *file, long const line);
	[[noreturn]] constexpr inline void assertion_failure_(char const *file, long const line, char const *assertion);
	[[noreturn]] constexpr inline void assertion_failure_(char const *file, long const line, std::string const &assertion);


	template <typename ... t_args>
	[[noreturn]] constexpr inline void assertion_failure_fmt_(char const *file, long const line, std::format_string <t_args...> fmt, t_args && ... args)
	{
		assertion_failure_(file, line, std::format(fmt, std::forward <t_args>(args)...));
	}


	template <bool t_requires_formatted_output_fn, typename t_lhs, typename t_rhs>
	[[noreturn]] constexpr void assertion_failure_(char const *file, long const line, t_lhs const &lhs, t_rhs const &rhs, char const *message)
	requires (!t_requires_formatted_output_fn || (has_formatted_output_function_v <t_lhs> && has_formatted_output_function_v <t_rhs>))
	{
		// Clang++ 14 is not happy about the use of std::stringstream in a constexpr function even if in the false branch
		// of if consteval, so we build the message here instead.
		if constexpr (has_formatted_output_function_v <t_lhs> && has_formatted_output_function_v <t_rhs>)
		{
			assertion_failure_fmt_(file, line, "{} (lhs: {}, rhs: {})", message, lhs, rhs);
		}
		else
		{
			assertion_failure_(file, line, message);
		}
	}


	template <typename t_line, typename ... t_args>
	constexpr inline void assert_test(bool const test, char const *file, t_line const line, std::format_string <t_args...> fmt, t_args && ... args)
	{
		if (!test)
			assertion_failure_fmt_(file, line, fmt, std::forward <t_args>(args)...);
	}


	template <typename t_test, bool t_requires_formatted_output_fn = false, typename t_lhs, typename t_rhs, typename t_line>
	constexpr inline void assert_test_bin(t_lhs &&lhs, t_rhs &&rhs, char const *file, t_line const line, char const *message)
	{
		t_test test;
		if (!test(lhs, rhs))
			assertion_failure_ <t_requires_formatted_output_fn>(file, line, lhs, rhs, message);
	}
}


namespace libbio {

	constexpr extern inline void assertion_failure() {} // For debugging.


	class assertion_failure_exception : public std::exception
	{
	protected:
		std::shared_ptr <detail::assertion_failure_cause>	m_cause;

	public:
		assertion_failure_exception() = default;

		assertion_failure_exception(char const *file, long line, std::string const &reason):
			m_cause(new detail::assertion_failure_cause(file, line, reason))
		{
		}

		assertion_failure_exception(char const *file, long line, std::string &&reason):
			m_cause(new detail::assertion_failure_cause(file, line, std::move(reason)))
		{
		}

		assertion_failure_exception(char const *file, long line):
			m_cause(new detail::assertion_failure_cause(file, line))
		{
		}

		virtual char const *what() const noexcept override { return m_cause->what.c_str(); }
		std::string const &file() const noexcept { return m_cause->file; }
		std::string const &reason() const noexcept { return m_cause->reason; }
		long line() const noexcept { return m_cause->line; }
	};
}


namespace libbio::detail {

#ifdef LIBBIO_USE_BOOST_STACKTRACE
	typedef boost::error_info <struct tag_stacktrace, boost::stacktrace::stacktrace> traced;

	template <typename t_exception, typename ... t_args>
	[[noreturn]] constexpr void do_throw(t_args && ... args)
	{
		throw boost::enable_error_info(t_exception(std::forward <t_args>(args)...)) << traced(boost::stacktrace::stacktrace());
	}
#else
	template <typename t_exception, typename ... t_args>
	[[noreturn]] constexpr void do_throw(t_args && ... args)
	{
		throw t_exception(std::forward <t_args>(args)...);
	}
#endif


	[[noreturn]] constexpr void assertion_failure_(char const *file, long const line)
	{
		::libbio::assertion_failure();
		do_throw <assertion_failure_exception>(file, line);
	}


	[[noreturn]] constexpr void assertion_failure_(char const *file, long const line, char const *assertion)
	{
		::libbio::assertion_failure();
		do_throw <assertion_failure_exception>(file, line, assertion);
	}


	[[noreturn]] constexpr void assertion_failure_(char const *file, long const line, std::string const &assertion)
	{
		::libbio::assertion_failure();
		do_throw <assertion_failure_exception>(file, line, assertion);
	}
}

#endif

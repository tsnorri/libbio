/*
 * Copyright (c) 2017-2025 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_ASSERT_HH
#define LIBBIO_ASSERT_HH

#include <cstddef>
#include <exception>
#include <format>
#include <libbio/tuple/access.hh>
#include <libbio/tuple/cat.hh>
#include <libbio/tuple/map.hh>
#include <libbio/utility/is_equal.hh>
#include <libbio/utility/is_lt.hh>
#include <libbio/utility/is_lte.hh>
#include <memory>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

#ifdef LIBBIO_USE_BOOST_STACKTRACE
#	include <boost/stacktrace.hpp>
#	include <boost/exception/all.hpp>
#endif

#define LIBBIO_UNCONDITIONAL_FAILURE_FMT	"Assertion failure in {}:{}"
#define LIBBIO_ASSERTION_FAILURE_FMT		"Assertion failure in {}:{}: test: {}"
#define LIBBIO_ASSERTION_FAILURE_BIN_FMT	"Assertion failure in {}:{}: test: {}, lhs: {}, rhs: {}"

namespace libbio::detail {
	// Types of the first three parameters in LIBBIO_ASSERTION_FAILURE_FMT and LIBBIO_ASSERTION_FAILURE_BIN_FMT.
	template <typename t_line>
	using assertion_failure_format_head_t = std::tuple <char const * &, t_line const &, char const * &>;
}


#define libbio_stringify(X) (#X)

// __VA_OPT__ is available in GCC from version 8.1 in all language modes, not just C++20.
#define libbio_assert_test(TEST, TEST_STR, ...)					do { ::libbio::detail::assert_test(TEST, __FILE__, __LINE__, TEST_STR, LIBBIO_ASSERTION_FAILURE_FMT __VA_OPT__(", ") __VA_ARGS__); } while (false)
#define libbio_assert_test_bin(LHS, RHS, TEST, TEST_STR, ...)	do { ::libbio::detail::assert_test_bin <TEST>(LHS, RHS, __FILE__, __LINE__, TEST_STR, LIBBIO_ASSERTION_FAILURE_BIN_FMT __VA_OPT__(", ") __VA_ARGS__); } while (false)

// FIXME: Make this work when immediately evaluated.
#define libbio_fail(...) do { \
		::libbio::detail::fail_unconditionally(__FILE__, __LINE__, LIBBIO_UNCONDITIONAL_FAILURE_FMT __VA_OPT__(", ") __VA_ARGS__); \
	} while (false)

#define libbio_always_assert(X, ...)			libbio_assert_test(bool(X),										#X,							## __VA_ARGS__)
#define libbio_always_assert_lt(X, Y, ...)		libbio_assert_test_bin((X), (Y), ::libbio::is_lt_,				libbio_stringify(X <  Y),	## __VA_ARGS__)
#define libbio_always_assert_lte(X, Y, ...)		libbio_assert_test_bin((X), (Y), ::libbio::is_lte_,				libbio_stringify(X <= Y),	## __VA_ARGS__)
#define libbio_always_assert_gt(X, Y, ...)		libbio_assert_test_bin((X), (Y), ::libbio::is_not_lte_,			libbio_stringify(X >  Y),	## __VA_ARGS__)
#define libbio_always_assert_gte(X, Y, ...)		libbio_assert_test_bin((X), (Y), ::libbio::is_not_lt_,			libbio_stringify(X >= Y),	## __VA_ARGS__)
#define libbio_always_assert_eq(X, Y, ...)		libbio_assert_test_bin((X), (Y), ::libbio::is_equal_,			libbio_stringify(X == Y),	## __VA_ARGS__)
#define libbio_always_assert_neq(X, Y, ...)		libbio_assert_test_bin((X), (Y), ::libbio::is_not_equal_,		libbio_stringify(X != Y),	## __VA_ARGS__)

#ifdef LIBBIO_NDEBUG
#	define libbio_assert(X, ...)
#	define libbio_assert_lt(X, Y, ...)
#	define libbio_assert_lte(X, Y, ...)
#	define libbio_assert_gt(X, Y, ...)
#	define libbio_assert_gte(X, Y, ...)
#	define libbio_assert_eq(X, Y, ...)
#	define libbio_assert_neq(X, Y, ...)
#else
#	define libbio_do_and_assert_eq(X, Y, ...)	libbio_always_assert_eq((X), (Y), ## __VA_ARGS__)
#	define libbio_assert(X, ...)				libbio_always_assert((X), ## __VA_ARGS__)
#	define libbio_assert_lt(X, Y, ...)			libbio_always_assert_lt((X), (Y), ## __VA_ARGS__)
#	define libbio_assert_lte(X, Y, ...)			libbio_always_assert_lte((X), (Y), ## __VA_ARGS__)
#	define libbio_assert_gt(X, Y, ...)			libbio_always_assert_gt((X), (Y), ## __VA_ARGS__)
#	define libbio_assert_gte(X, Y, ...)			libbio_always_assert_gte((X), (Y), ## __VA_ARGS__)
#	define libbio_assert_eq(X, Y, ...)			libbio_always_assert_eq((X), (Y), ## __VA_ARGS__)
#	define libbio_assert_neq(X, Y, ...)			libbio_always_assert_neq((X), (Y), ## __VA_ARGS__)
#endif


namespace libbio::detail {

	// Copying a standard library class derived from std::exception
	// is not permitted to throw exceptions, so try to avoid it here, too.
	struct assertion_failure_cause
	{
		std::string		reason;
		std::string		file;
		long			line{};

		assertion_failure_cause() = default;

		assertion_failure_cause(char const *file_, long line_):
			file{file_},
			line{line_}
		{
		}

		assertion_failure_cause(char const *file_, long line_, std::string &&reason_):
			reason{std::move(reason_)},
			file{file_},
			line{line_}
		{
		}
	};


	template <typename t_type>
	using assertion_failure_format_value_tuple_t = std::tuple <
		std::conditional_t <
			std::formattable <t_type, char>,
			t_type const &,
			char const * const &
		>
	>;

	// For some reason outputting std::byte const * with std::format does not work.
	// (Perhaps the rationale is that formatting library cannot know if it should
	// apply string formatting or output the pointer value.) Hence we cast to a
	// void * or void const * here.
	template <typename t_type, typename t_type_ = std::remove_cvref_t <t_type>>
	struct process_assertion_format_parameter_trait
	{
		typedef t_type type;
	};

	template <typename t_type>
	struct process_assertion_format_parameter_trait <t_type, std::byte *>
	{
		typedef void const * type;
	};

	template <typename t_type>
	struct process_assertion_format_parameter_trait <t_type, std::byte const *>
	{
		typedef void const * type;
	};

	template <typename t_type>
	using process_assertion_format_parameter_t = process_assertion_format_parameter_trait <t_type>::type;


	template <typename t_line, typename ... t_args>
	using assertion_format_string_t = tuples::forward_to <std::format_string>::parameters_of_t <
		tuples::cat_t <
			assertion_failure_format_head_t <t_line>,
			tuples::map_t <std::tuple <t_args...>, process_assertion_format_parameter_t>
		>
	>;

	template <typename t_line, typename t_lhs, typename t_rhs, typename ... t_args>
	using assertion_format_string_bin_t = tuples::forward_to <std::format_string>::parameters_of_t <
		tuples::cat_t <
			assertion_failure_format_head_t <t_line>,
			assertion_failure_format_value_tuple_t <t_lhs>,
			assertion_failure_format_value_tuple_t <t_rhs>,
			tuples::map_t <std::tuple <t_args...>, process_assertion_format_parameter_t>
		>
	>;


	template <typename t_type>
	decltype(auto) process_for_assertion_format(t_type &&val)
	requires (!std::is_same_v <std::remove_cvref_t <t_type>, std::byte *>) { return val; }

	inline void const *process_for_assertion_format(std::byte const *ptr) { return ptr; }


	// Fwd.
	[[noreturn]] constexpr inline void assertion_failure_(char const *file, long const line, std::string &&message);
	[[noreturn]] constexpr inline void assertion_failure_(char const *file, long const line, char const *assertion, std::string &&message);


	template <typename t_line, typename ... t_args>
	[[noreturn]] constexpr inline void fail_unconditionally(char const *file, t_line const line, std::format_string <char const * &, t_line const &, t_args...> fmt, t_args && ... args)
	{
		assertion_failure_(file, line, std::format(fmt, file, line, std::forward <t_args>(args)...));
	}


	// Check the given boolean value.
	template <typename t_line, typename ... t_args>
	constexpr inline void assert_test(
		bool const test,
		char const *file,
		t_line const line,
		char const *test_str,
		assertion_format_string_t <t_line, t_args...> fmt,
		t_args && ... args
	)
	{
		if (!test)
			assertion_failure_(file, line, test_str, std::format(fmt, file, line, test_str, process_for_assertion_format(std::forward <t_args>(args))...));
	}


	// Test with the given binary operator.
	template <typename t_test, typename t_lhs, typename t_rhs, typename t_line, typename ... t_args>
	constexpr inline void assert_test_bin(
		t_lhs const &lhs,
		t_rhs const &rhs,
		char const *file,
		t_line const line,
		char const *test_str,
		assertion_format_string_bin_t <t_line, t_lhs, t_rhs, t_args...> fmt,
		t_args && ... args
	)
	{
		t_test test;
		if (!test(lhs, rhs))
		{
			auto const check_formattable([&]<typename t_value>(t_value &&value, auto &&cb){
				if constexpr (std::formattable <t_value, char>)
					cb(value);
				else
					cb(static_cast <char const *>("(not formattable)"));
			});

			check_formattable(lhs, [&](auto const &lhs_){
				check_formattable(rhs, [&](auto const &rhs_){
					assertion_failure_(
						file,
						line,
						test_str,
						std::format(
							fmt,
							file,
							line,
							test_str,
							process_for_assertion_format(lhs_),
							process_for_assertion_format(rhs_),
							process_for_assertion_format(std::forward <t_args>(args))...
						)
					);
				});
			});
		}
	}
}


namespace libbio {

	constexpr inline bool all_assertions_enabled()
	{
#if defined(LIBBIO_NDEBUG) && LIBBIO_NDEBUG
		return false;
#else
		return true;
#endif
	}


	constexpr extern inline void assertion_failure() {} // For debugging.


	class assertion_failure_exception : public std::exception
	{
	protected:
		std::shared_ptr <detail::assertion_failure_cause>	m_cause;
		char const											*m_assertion{};

	public:
		assertion_failure_exception() = default;

		assertion_failure_exception(char const *file, long line, char const *assertion, std::string &&reason):
			m_cause{new detail::assertion_failure_cause(file, line, std::move(reason))},
			m_assertion{assertion}
		{
		}

		assertion_failure_exception(char const *file, long line, std::string &&reason):
			m_cause{new detail::assertion_failure_cause(file, line, std::move(reason))}
		{
		}

		virtual char const *what() const noexcept override { auto *cause{m_cause.get()}; return (cause ? cause->reason.c_str() : nullptr); }
		char const *assertion() const { return m_assertion; }
		std::string const &file() const noexcept { return m_cause->file; }
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


	[[noreturn]] constexpr void assertion_failure_(char const *file, long const line, std::string &&message)
	{
		::libbio::assertion_failure();
		do_throw <assertion_failure_exception>(file, line, std::move(message));
	}


	[[noreturn]] constexpr void assertion_failure_(char const *file, long const line, char const *assertion, std::string &&message)
	{
		::libbio::assertion_failure();
		do_throw <assertion_failure_exception>(file, line, assertion, std::move(message));
	}
}

#endif

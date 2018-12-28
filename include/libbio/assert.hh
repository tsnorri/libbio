/*
 * Copyright (c) 2017-2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_ASSERT_HH
#define LIBBIO_ASSERT_HH

#include <iostream>
#include <libbio/buffer.hh>
#include <libbio/utility/is_equal.hh>
#include <libbio/utility/is_lte.hh>
#include <libbio/utility/misc.hh>
#include <stdexcept>

#define libbio_stringify(X) (#X)


// Contracts not yet available in either Clang or GCC.
#define libbio_fail(MESSAGE)				do { ::libbio::detail::fail(__FILE__, __LINE__, MESSAGE); } while (false)
#define libbio_always_assert(X)				do { \
		if (!(X)) ::libbio::detail::assertion_failure(__FILE__, __LINE__, #X); \
	} while (false)

#ifdef LIBBIO_NDEBUG
#	define libbio_assert(...)
#	define libbio_assert_lte(X, Y)
#	define libbio_assert_eq(X, Y)
#	define libbio_do_and_assert_eq(X, Y)	do { (X); } while (false)
#else
#	define libbio_assert(X)					do { \
		if (!(X)) ::libbio::detail::assertion_failure(__FILE__, __LINE__, #X); \
	} while (false)
#	define libbio_assert_lte(X, Y)			do { \
		if (!::libbio::is_lte(X, Y))	::libbio::detail::assertion_failure(__FILE__, __LINE__, libbio_stringify(X <= Y)); \
	} while (false)
#	define libbio_assert_eq(X, Y)			do { \
		if (!::libbio::is_equal(X, Y))	::libbio::detail::assertion_failure(__FILE__, __LINE__, libbio_stringify(X == Y)); \
	} while (false)
#	define libbio_do_and_assert_eq(X, Y)	do { \
		if (!::libbio::is_equal(X, Y))	::libbio::detail::assertion_failure(__FILE__, __LINE__, libbio_stringify(X == Y)); \
	} while (false)
#endif


namespace libbio { namespace detail {

	// Copying a standard library class derived from std::exception
	// is not permitted to throw exceptions, so try to avoid it here, too.
	struct assertion_failure_cause
	{
		std::string		reason;
		std::string		file;
		buffer <char>	what;
		int				line{};

		assertion_failure_cause() = default;

		assertion_failure_cause(char const *file_, int line_):
			file(file_),
			what(copy_format_cstr(boost::format("%s:%d") % file % line_)),
			line(line_)
		{
		}

		assertion_failure_cause(char const *file_, int line_, std::string &&reason_):
			reason(std::move(reason_)),
			file(file_),
			what(copy_format_cstr(boost::format("%s:%d: %s") % file % line_ % reason)),
			line(line_)
		{
		}
	};
	
	
	void assertion_failure(char const *file, long const line, char const *assertion);
	void fail(char const *file, long const line);
}}


namespace libbio {
	
	class assertion_failure_exception final : public std::exception
	{
	protected:
		std::shared_ptr <detail::assertion_failure_cause>	m_cause;

	public:
		assertion_failure_exception() = default;

		assertion_failure_exception(char const *file, int line, std::string &&reason):
			m_cause(new detail::assertion_failure_cause(file, line, std::move(reason)))
		{
		}

		assertion_failure_exception(char const *file, int line):
			m_cause(new detail::assertion_failure_cause(file, line))
		{
		}

		virtual char const *what() const noexcept override { return m_cause->what.get(); }
		std::string const &file() const noexcept { return m_cause->file; }
		std::string const &reason() const noexcept { return m_cause->reason; }
		int line() const noexcept { return m_cause->line; }
	};
}

#endif

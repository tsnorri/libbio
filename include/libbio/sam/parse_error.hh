/*
 * Copyright (c) 2023-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_SAM_PARSE_ERROR_HH
#define LIBBIO_SAM_PARSE_ERROR_HH

#include <exception>
#include <string>
#include <string_view>


namespace libbio::sam {

	class parse_error final : public std::exception
	{
	protected:
		std::string	m_reason{};
		std::string	m_value{};

	public:
		using std::exception::exception;

		parse_error(char const *reason):
			m_reason(reason)
		{
		}

		parse_error(char const *reason, std::string_view const value):
			m_reason(reason),
			m_value(value)
		{
		}

		char const *what() const noexcept override { return m_reason.data(); }
		std::string const &value() const { return m_value; }
	};
}

#endif

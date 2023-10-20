/*
 * Copyright (c) 2022 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_GENERIC_PARSER_ERRORS_HH
#define LIBBIO_GENERIC_PARSER_ERRORS_HH

#include <any>
#include <exception>
#include <libbio/generic_parser/field_position.hh>
#include <libbio/generic_parser/fwd.hh>
#include <libbio/generic_parser/utility.hh>
#include <ostream>


namespace libbio::parsing::errors {
	
	struct unexpected_eof
	{
		char const *what() const noexcept { return "Unexpected EOF"; }
	};
	
	
	template <typename t_value>
	struct unexpected_character
	{
		t_value value{};
		
		explicit unexpected_character(t_value const value_):
			value(value_)
		{
		}
		
		char const *what() const noexcept { return "Unexpected character"; }
	};
	
	
	inline std::ostream &operator<<(std::ostream &os, unexpected_eof const &)
	{
		os << "Unexpected EOF";
		return os;
	}
	
	
	template <typename t_value>
	std::ostream &operator<<(std::ostream &os, unexpected_character <t_value> const &err)
	{
		os << "Unexpected character ";
		
		if constexpr (detail::is_character_type_v <t_value>)
		{
			if (detail::is_printable(err.value))
				os << "‘" << err.value << "’ (" << (+err.value) << ')';
			else
				os << (+err.value);
		}
		else
		{
			os << err.value;
		}
		
		return os;
	}
}


namespace libbio::parsing {
	
	class parse_error : public std::exception
	{
		template <typename, bool, typename...>
		friend class parser_tpl;
		
	protected:
		std::any			m_expected_delimiter{};
		std::size_t			m_field_index{SIZE_MAX};
		std::size_t			m_position{SIZE_MAX};
		enum field_position	m_field_position{field_position::none_};

	public:
		virtual ~parse_error() {}
		std::size_t position() const { return m_position; }
		virtual void output_error(std::ostream &os) const = 0;
	};
	
	
	template <typename t_error>
	class parse_error_tpl final : public parse_error
	{
	protected:
		t_error	m_error{};
		
	public:
		explicit parse_error_tpl(t_error &&error):
			m_error(std::move(error))
		{
		}
		
		char const *what() const noexcept override { return m_error.what(); }
		void output_error(std::ostream &os) const override { os << m_error; }
	};
	
	
	inline std::ostream &operator<<(std::ostream &os, parse_error const &error)
	{
		error.output_error(os);
		return os;
	}
}

#endif

/*
 * Copyright (c) 2022-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_PARSE_ERROR_HH
#define LIBBIO_VCF_PARSE_ERROR_HH

#include <exception>
#include <libbio/vcf/metadata.hh>
#include <string>
#include <string_view>


namespace libbio::vcf {

	class parse_error final : public std::exception
	{
	protected:
		std::string						m_reason{};
		std::string_view				m_value{};
		metadata_formatted_field const	*m_field{};
		bool							m_has_value{};

	public:
		using std::exception::exception;

		parse_error(char const *reason):
			m_reason(reason)
		{
		}

		parse_error(char const *reason, std::string_view const value):
			m_reason(reason),
			m_value(value),
			m_has_value(true)
		{
		}

		parse_error(char const *reason, std::string_view const value, metadata_formatted_field const *field):
			m_reason(reason),
			m_value(value),
			m_field(field),
			m_has_value(true)
		{
		}

		char const *what() const noexcept override { return m_reason.data(); }
		std::string_view const *value() const { return (m_has_value ? &m_value : nullptr); }
		metadata_formatted_field const *get_metadata() const { return m_field; }
	};
}

#endif

/*
 * Copyright (c) 2020-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_VARIANT_VARIANT_DECL_HH
#define LIBBIO_VCF_VARIANT_VARIANT_DECL_HH

#include <libbio/vcf/variant/formatted_variant_decl.hh>
#include <libbio/vcf/variant/fwd.hh>
#include <string>
#include <string_view>


namespace libbio::vcf {

	class transient_variant final : public transient_variant_formatted_base
	{
		friend class variant;

	public:
		typedef std::string_view						string_type;
		typedef detail::transient_variant_format_access	format_access;
		typedef transient_variant_sample				sample_type;

	public:
		using transient_variant_formatted_base::transient_variant_formatted_base;

		transient_variant(transient_variant const &) = delete;
		transient_variant(transient_variant &&) = default;
		transient_variant &operator=(transient_variant const &) = delete;
		transient_variant &operator=(transient_variant &&) & = default;

		inline void reset();
	};


	class variant final : public variant_formatted_base
	{
	public:
		typedef std::string						string_type;
		typedef detail::variant_format_access	format_access;
		typedef variant_sample					sample_type;

	public:
		using variant_formatted_base::variant_formatted_base;
		inline variant &operator=(transient_variant const &other) &;
	};
}

#endif

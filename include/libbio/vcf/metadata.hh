/*
 * Copyright (c) 2019-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_METADATA_HH
#define LIBBIO_VCF_METADATA_HH

#include <cstdint>
#include <libbio/utility/compare_strings_transparent.hh>
#include <libbio/vcf/constants.hh>
#include <map>
#include <ostream>
#include <range/v3/view/transform.hpp>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>


namespace libbio::vcf::detail {
	class metadata_setup_helper; // Fwd.
}


namespace libbio::vcf {

	class transient_variant;


#define LIBBIO_VCF_METADATA_STR_FIELD(NAME) \
	public: \
		virtual void set_##NAME(std::string_view const &sv) final { m_##NAME = sv; } \
		std::string const &get_##NAME() const { return m_##NAME; } \
	protected: std::string m_##NAME

#define LIBBIO_VCF_METADATA_INT32_FIELD(NAME) \
	public: \
		virtual void set_##NAME(std::int32_t const val) final { m_##NAME = val; } \
		int32_t get_##NAME() const { return m_##NAME; } \
	protected: std::int32_t m_##NAME

#define LIBBIO_VCF_METADATA_NUMBER_FIELD \
	public: \
		virtual void set_number(std::int32_t const val) final		{ m_number = val; } \
		virtual void set_number_one_per_alternate_allele() final	{ m_number = VCF_NUMBER_ONE_PER_ALTERNATE_ALLELE; } \
		virtual void set_number_one_per_allele() final				{ m_number = VCF_NUMBER_ONE_PER_ALLELE; } \
		virtual void set_number_one_per_genotype() final			{ m_number = VCF_NUMBER_ONE_PER_GENOTYPE; } \
		virtual void set_number_unknown() final						{ m_number = VCF_NUMBER_UNKNOWN; } \
		int32_t const &get_number() const							{ return m_number; } \
	protected: std::int32_t m_number

#define LIBBIO_VCF_METADATA_VALUE_TYPE_FIELD \
	public: \
		virtual void set_value_type(metadata_value_type const vt) final	{ m_value_type = vt; } \
		metadata_value_type get_value_type() const						{ return m_value_type; } \
	protected: metadata_value_type m_value_type{metadata_value_type::UNKNOWN}


	class metadata_base
	{
		friend class reader;

	protected:
		std::uint16_t	m_header_index{};
		std::uint16_t	m_index{};

	public:
		constexpr std::uint16_t get_header_index() const { return m_header_index; }
		constexpr std::uint16_t get_index() const { return m_index; }

	public:
		virtual ~metadata_base() {}

	protected:
		virtual void set_id(std::string_view const &sv)					{ throw std::runtime_error("Not implemented"); }
		virtual void set_description(std::string_view const &sv)		{ throw std::runtime_error("Not implemented"); }
		virtual void set_source(std::string_view const &sv)				{ throw std::runtime_error("Not implemented"); }
		virtual void set_version(std::string_view const &sv)			{ throw std::runtime_error("Not implemented"); }
		virtual void set_url(std::string_view const &sv)				{ throw std::runtime_error("Not implemented"); }
		virtual void set_length(std::int32_t const val)					{ throw std::runtime_error("Not implemented"); }
		virtual void set_assembly(std::string_view const &sv)			{ throw std::runtime_error("Not implemented"); }
		virtual void set_md5(std::string_view const &sv)				{ throw std::runtime_error("Not implemented"); }
		virtual void set_taxonomy(std::string_view const &sv)			{ throw std::runtime_error("Not implemented"); }
		virtual void set_species(std::string_view const &sv)			{ throw std::runtime_error("Not implemented"); }

		virtual void set_number(std::int32_t const val)					{ throw std::runtime_error("Not implemented"); }
		virtual void set_number_one_per_alternate_allele()				{ throw std::runtime_error("Not implemented"); }
		virtual void set_number_one_per_allele()						{ throw std::runtime_error("Not implemented"); }
		virtual void set_number_one_per_genotype()						{ throw std::runtime_error("Not implemented"); }
		virtual void set_number_unknown()								{ throw std::runtime_error("Not implemented"); }

		virtual void set_value_type(metadata_value_type const vt)		{ throw std::runtime_error("Not implemented"); }

	public:
		virtual metadata_type type() const = 0;
		virtual void output_vcf(std::ostream &stream) const = 0;
	};


	class metadata_formatted_field : public metadata_base
	{
		friend class reader;
		friend class info_field_base;
		friend class genotype_field_base;
		friend class detail::metadata_setup_helper;

	protected:
		template <typename t_field>
		void check_field(t_field const &field) const { check_field(field.number(), field.metadata_value_type()); }

		void check_field(std::int32_t const number, metadata_value_type const vt) const;

		LIBBIO_VCF_METADATA_STR_FIELD(id);
		LIBBIO_VCF_METADATA_STR_FIELD(description);
		LIBBIO_VCF_METADATA_NUMBER_FIELD;
		LIBBIO_VCF_METADATA_VALUE_TYPE_FIELD;
	};


	inline std::ostream &operator<<(std::ostream &os, metadata_formatted_field const &ff)
	{
		os << "ID: " << ff.get_id();
		os << " Number: ";
		output_vcf_value(os, ff.get_number());
		os << " Value type: ";
		output_vcf_value(os, ff.get_value_type());
		os << " Description: " << ff.get_description();
		return os;
	}


	class metadata_info final : public metadata_formatted_field
	{
		friend class reader;
		friend class metadata;

	protected:
		LIBBIO_VCF_METADATA_STR_FIELD(source);
		LIBBIO_VCF_METADATA_STR_FIELD(version);

	public:
		virtual metadata_type type() const override { return metadata_type::INFO; }
		virtual void output_vcf(std::ostream &stream) const override;

	protected:
		inline bool check_subfield_index(std::int32_t subfield_index) const;
	};


	class metadata_format final : public metadata_formatted_field
	{
		friend class reader;

	public:
		virtual metadata_type type() const override { return metadata_type::FORMAT; }
		virtual void output_vcf(std::ostream &stream) const override;
	};


	class metadata_filter final : public metadata_base
	{
		LIBBIO_VCF_METADATA_STR_FIELD(id);
		LIBBIO_VCF_METADATA_STR_FIELD(description);

	public:
		virtual metadata_type type() const override { return metadata_type::FILTER; }
		virtual void output_vcf(std::ostream &stream) const override;
	};


	class metadata_alt final : public metadata_base
	{
		LIBBIO_VCF_METADATA_STR_FIELD(id);
		LIBBIO_VCF_METADATA_STR_FIELD(description);

	public:
		virtual metadata_type type() const override { return metadata_type::ALT; }
		virtual void output_vcf(std::ostream &stream) const override;
	};


	class metadata_assembly final : public metadata_base
	{
		LIBBIO_VCF_METADATA_STR_FIELD(assembly);

	public:
		virtual metadata_type type() const override { return metadata_type::ASSEMBLY; }
		virtual void output_vcf(std::ostream &stream) const override;
	};


	class metadata_contig final : public metadata_base
	{
		LIBBIO_VCF_METADATA_STR_FIELD(id);
		LIBBIO_VCF_METADATA_INT32_FIELD(length);

	public:
		virtual metadata_type type() const override { return metadata_type::CONTIG; }
		virtual void output_vcf(std::ostream &stream) const override;
	};


	typedef std::variant <
		metadata_info,
		metadata_format,
		metadata_filter,
		metadata_alt,
		metadata_assembly,
		metadata_contig
	> metadata_record_var;


#undef LIBBIO_VCF_METADATA_STR_FIELD
#undef LIBBIO_VCF_METADATA_INT32_FIELD
#undef LIBBIO_VCF_METADATA_NUMBER_FIELD
#undef LIBBIO_VCF_METADATA_VALUE_TYPE_FIELD


	class metadata
	{
		friend class reader;

	public:
		typedef std::map <std::string,	metadata_info,		libbio::compare_strings_transparent>	info_map;	// By ID
		typedef std::map <std::string,	metadata_filter,	libbio::compare_strings_transparent>	filter_map;	// By ID
		typedef std::map <std::string,	metadata_format,	libbio::compare_strings_transparent>	format_map;	// By ID
		typedef std::map <std::string,	metadata_alt,		libbio::compare_strings_transparent>	alt_map;	// By ALT ID
		typedef std::map <std::string,	metadata_contig,	libbio::compare_strings_transparent>	contig_map;	// By ID
		typedef std::vector <metadata_assembly>														assembly_vector;

	protected:
		info_map							m_info;		// By ID
		filter_map							m_filter;	// By ID
		format_map							m_format;	// By ID
		alt_map								m_alt;		// By ALT ID
		contig_map							m_contig;	// By ID
		assembly_vector						m_assembly;

	protected:
		void add_metadata(metadata_info &&m)		{ m_info.emplace(m.get_id(), std::move(m)); }
		void add_metadata(metadata_filter &&m)		{ m_filter.emplace(m.get_id(), std::move(m)); }
		void add_metadata(metadata_format &&m)		{ m_format.emplace(m.get_id(), std::move(m)); }
		void add_metadata(metadata_alt &&m)			{ m_alt.emplace(m.get_id(), std::move(m)); }
		void add_metadata(metadata_contig &&m)		{ m_contig.emplace(m.get_id(), std::move(m)); }
		void add_metadata(metadata_assembly &&m)	{ m_assembly.emplace_back(std::move(m)); }

		template <typename t_metadata, typename t_cmp, typename t_fn>
		void visit_metadata_map(std::map <std::string, t_metadata, t_cmp> const &metadata_map, t_fn &&cb) const
		{
			for (auto const &[key, metadata] : metadata_map)
				cb(metadata);
		}

	public:
		alt_map &alt() { return m_alt; }
		alt_map const &alt() const { return m_alt; }
		info_map &info() { return m_info; }
		info_map const &info() const { return m_info; }
		format_map &format() { return m_format; }
		format_map const &format() const { return m_format; }
		filter_map &filter() { return m_filter; }
		filter_map const &filter() const { return m_filter; }
		contig_map &contig() { return m_contig; }
		contig_map const &contig() const { return m_contig; }
		assembly_vector &assembly() { return m_assembly; }
		assembly_vector const &assembly() const { return m_assembly; }

		template <typename t_fn>
		void visit_all_metadata(t_fn &&cb) const
		{
			for (auto const &assembly : m_assembly)
				cb(assembly);

			visit_metadata_map(m_info, cb);
			visit_metadata_map(m_filter, cb);
			visit_metadata_map(m_format, cb);
			visit_metadata_map(m_alt, cb);
			visit_metadata_map(m_contig, cb);
		}
	};


	bool metadata_info::check_subfield_index(std::int32_t subfield_index) const
	{
		switch (m_number)
		{
			case VCF_NUMBER_UNKNOWN:
			case VCF_NUMBER_ONE_PER_ALTERNATE_ALLELE:
			case VCF_NUMBER_ONE_PER_ALLELE:
			case VCF_NUMBER_ONE_PER_GENOTYPE:
				return true;

			default:
				return (subfield_index < m_number);
		}
	}
}

#endif

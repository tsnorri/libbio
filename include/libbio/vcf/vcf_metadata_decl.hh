/*
 * Copyright (c) 2019 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_METADATA_DECL_HH
#define LIBBIO_VCF_METADATA_DECL_HH

#include <libbio/cxxcompat.hh>
#include <libbio/utility/compare_strings_transparent.hh>
#include <libbio/vcf/vcf_reader_support.hh>
#include <libbio/vcf/vcf_subfield_decl.hh>
#include <map>
#include <ostream>
#include <range/v3/view/transform.hpp>
#include <variant>
#include <vector>


namespace libbio {
	
	class transient_variant;
	
	
#define VCF_METADATA_STR_FIELD(NAME) \
	public: \
		virtual void set_##NAME(std::string_view const &sv) override { m_##NAME = sv; } \
		std::string const &get_##NAME() const { return m_##NAME; } \
	protected: std::string m_##NAME

#define VCF_METADATA_INT32_FIELD(NAME) \
	public: \
		virtual void set_##NAME(std::int32_t const val) override { m_##NAME = val; } \
		int32_t const get_##NAME() const { return m_##NAME; } \
	protected: std::int32_t m_##NAME

#define VCF_METADATA_NUMBER_FIELD \
	public: \
		virtual void set_number(std::int32_t const val) override	{ m_number = val; } \
		virtual void set_number_one_per_alternate_allele() override { m_number = VCF_NUMBER_ONE_PER_ALTERNATE_ALLELE; } \
		virtual void set_number_one_per_allele() override			{ m_number = VCF_NUMBER_ONE_PER_ALLELE; } \
		virtual void set_number_one_per_genotype() override			{ m_number = VCF_NUMBER_ONE_PER_GENOTYPE; } \
		virtual void set_number_unknown() override					{ m_number = VCF_NUMBER_UNKNOWN; } \
		int32_t const &get_number() const							{ return m_number; } \
	protected: std::int32_t m_number

#define VCF_METADATA_VALUE_TYPE_FIELD \
	public: \
		virtual void set_value_type(vcf_metadata_value_type const vt) override	{ m_value_type = vt; } \
		vcf_metadata_value_type get_value_type() const							{ return m_value_type; } \
	protected: vcf_metadata_value_type m_value_type{vcf_metadata_value_type::UNKNOWN}
	
	
	class vcf_metadata_base
	{
		friend class vcf_reader;
		
	public:
		virtual ~vcf_metadata_base() {}
		
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
		
		virtual void set_value_type(vcf_metadata_value_type const vt)	{ throw std::runtime_error("Not implemented"); }
		
	public:
		virtual void output_vcf(std::ostream &stream) const = 0;
	};
	
	
	class vcf_metadata_formatted_field : public vcf_metadata_base
	{
	protected:
		inline void check_field_type(vcf_metadata_value_type const vt) const;
	};
	
	
	class vcf_metadata_info final : public vcf_metadata_formatted_field
	{
		friend class vcf_reader;
		friend class vcf_metadata;
		
	protected:
		// The corresponding field data structure.
		vcf_info_field_base	*m_field{&vcf_reader_support::get_instance().get_info_field_placeholder()};
		std::size_t			m_index{};
		
		VCF_METADATA_STR_FIELD(id);
		VCF_METADATA_STR_FIELD(description);
		VCF_METADATA_STR_FIELD(source);
		VCF_METADATA_STR_FIELD(version);
		VCF_METADATA_NUMBER_FIELD;
		VCF_METADATA_VALUE_TYPE_FIELD;
		
	public:
		std::size_t get_index() const { return m_index; }
		vcf_info_field_base &get_field() { return *m_field; }
		virtual void output_vcf(std::ostream &stream) const override;
		bool output_vcf_field(std::ostream &stream, variant_base const &var, char const *sep) const;
		
	protected:
		inline bool check_subfield_index(std::int32_t subfield_index) const;
		inline void prepare(variant_base &dst) const;
		inline void parse_and_assign(std::string_view const &sv, transient_variant &dst) const;
		inline void assign_flag(transient_variant &dst) const;
	};
	
	
	class vcf_metadata_format final : public vcf_metadata_formatted_field
	{
		friend class vcf_reader;
		
	protected:
		// The corresponding field data structure.
		vcf_genotype_field_base	*m_field{&vcf_reader_support::get_instance().get_genotype_field_placeholder()};
		
		VCF_METADATA_STR_FIELD(id);
		VCF_METADATA_STR_FIELD(description);
		VCF_METADATA_NUMBER_FIELD;
		VCF_METADATA_VALUE_TYPE_FIELD;
		
	public:
		vcf_genotype_field_base &get_field() const { return *m_field; }
		virtual void output_vcf(std::ostream &stream) const override;
		bool output_vcf_field(std::ostream &stream, variant_sample const &sample, char const *sep) const;
		
	protected:
		inline void prepare(variant_sample &dst) const;
		inline void parse_and_assign(std::string_view const &sv, variant_sample &dst) const;
	};
	
	typedef std::vector <vcf_metadata_format const *>	vcf_format_ptr_vector;
	
	
	class vcf_metadata_filter final : public vcf_metadata_base
	{
		VCF_METADATA_STR_FIELD(id);
		VCF_METADATA_STR_FIELD(description);
		
	public:
		virtual void output_vcf(std::ostream &stream) const override;
	};
	
	
	class vcf_metadata_alt final : public vcf_metadata_base
	{
		VCF_METADATA_STR_FIELD(id);
		VCF_METADATA_STR_FIELD(description);

	public:
		virtual void output_vcf(std::ostream &stream) const override;
	};
	
	
	class vcf_metadata_assembly final : public vcf_metadata_base
	{
		VCF_METADATA_STR_FIELD(assembly);
		
	public:
		virtual void output_vcf(std::ostream &stream) const override;
	};
	
	
	class vcf_metadata_contig final : public vcf_metadata_base
	{
		VCF_METADATA_STR_FIELD(id);
		VCF_METADATA_INT32_FIELD(length);
		
	public:
		virtual void output_vcf(std::ostream &stream) const override;
	};
	
	
	typedef std::variant <
		vcf_metadata_info,
		vcf_metadata_format,
		vcf_metadata_filter,
		vcf_metadata_alt,
		vcf_metadata_assembly,
		vcf_metadata_contig
	> vcf_metadata_record_var;
	
	
#undef VCF_METADATA_STR_FIELD
#undef VCF_METADATA_INT32_FIELD
#undef VCF_METADATA_NUMBER_FIELD
	
	
	class vcf_metadata
	{
		friend class vcf_reader;
		
	public:
		typedef std::map <std::string,	vcf_metadata_info,		libbio::compare_strings_transparent>	info_map;	// By ID
		typedef std::map <std::string,	vcf_metadata_filter,	libbio::compare_strings_transparent>	filter_map;	// By ID
		typedef std::map <std::string,	vcf_metadata_format,	libbio::compare_strings_transparent>	format_map;	// By ID
		typedef std::map <std::string,	vcf_metadata_alt,		libbio::compare_strings_transparent>	alt_map;	// By ALT ID
		typedef std::map <std::string,	vcf_metadata_contig,	libbio::compare_strings_transparent>	contig_map;	// By ID
		
	protected:
		info_map							m_info;		// By ID
		filter_map							m_filter;	// By ID
		format_map							m_format;	// By ID
		alt_map								m_alt;		// By ALT ID
		contig_map							m_contig;	// By ID
		std::vector <vcf_metadata_assembly>	m_assembly;
		std::size_t							m_info_idx{};
		
	protected:
		void add_metadata(vcf_metadata_info &&m)		{ m.m_index = m_info_idx++; m_info.emplace(m.get_id(), std::move(m)); }
		void add_metadata(vcf_metadata_filter &&m)		{ m_filter.emplace(m.get_id(), std::move(m)); }
		void add_metadata(vcf_metadata_format &&m)		{ m_format.emplace(m.get_id(), std::move(m)); }
		void add_metadata(vcf_metadata_alt &&m)			{ m_alt.emplace(m.get_id(), std::move(m)); }
		void add_metadata(vcf_metadata_contig &&m)		{ m_contig.emplace(m.get_id(), std::move(m)); }
		void add_metadata(vcf_metadata_assembly &&m)	{ m_assembly.emplace_back(std::move(m)); }
		
		template <typename t_metadata, typename t_cmp, typename t_fn>
		void visit_metadata_map(std::map <std::string, t_metadata, t_cmp> const &metadata_map, t_fn &&cb) const
		{
			for (auto const &[key, metadata] : metadata_map)
				cb(metadata);
		}
		
	public:
		info_map const &info() const { return m_info; }
		
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
}

#endif

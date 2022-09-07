/*
 * Copyright (c) 2019-2020 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_SUBFIELD_GENOTYPE_FIELD_GT_HH
#define LIBBIO_VCF_SUBFIELD_GENOTYPE_FIELD_GT_HH

#include <libbio/assert.hh>
#include <libbio/vcf/subfield/base.hh>
#include <libbio/vcf/subfield/container_access.hh>
#include <libbio/vcf/subfield/generic_field.hh>
#include <libbio/vcf/subfield/utility/value_access.hh>
#include <libbio/vcf/variant/sample.hh>
#include <ostream>


namespace libbio::vcf::detail {
	
	struct genotype_field_gt_base : public genotype_field_base
	{
	protected:
		typedef vector_value_access <sample_genotype, VCF_NUMBER_UNKNOWN>	value_access;
		
	public:
		template <bool B>
		using container_tpl = genotype_field_base::container_tpl <B>;
		
		template <bool B>
		using field_access_tpl = value_access;
	};
}


namespace libbio::vcf {
	
	// Subclass for the GT field.
	class genotype_field_gt final :	public detail::generic_field_tpl <detail::genotype_field_gt_base>
	{
	public:
		typedef detail::generic_field_tpl <genotype_field_gt_base>	base_class;
		typedef typename base_class::container_type					container_type;
		typedef typename base_class::transient_container_type		transient_container_type;
		
	protected:
		virtual bool parse_and_assign(std::string_view const &sv, transient_variant const &var, transient_variant_sample &sample, std::byte *mem) const override;
		virtual genotype_field_gt *clone() const override { return new genotype_field_gt(*this); }
		
	public:
		// As per VCF 4.3 specification Table 2.
		constexpr virtual enum metadata_value_type metadata_value_type() const override { return metadata_value_type::STRING; }
		constexpr virtual std::int32_t number() const override { return 1; }
		
		using detail::generic_field_tpl <genotype_field_gt_base>::output_vcf_value;
		virtual void output_vcf_value(std::ostream &stream, container_type const &ct) const override { output_genotype(stream, (*this)(ct)); }
		virtual void output_vcf_value(std::ostream &stream, transient_container_type const &ct) const override { output_genotype(stream, (*this)(ct)); }
	};
}

#endif

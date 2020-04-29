/*
 * Copyright (c) 2019-2020 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_SUBFIELD_GENOTYPE_FIELD_GT_HH
#define LIBBIO_VCF_SUBFIELD_GENOTYPE_FIELD_GT_HH

#include <libbio/assert.hh>
#include <libbio/vcf/subfield/base.hh>
#include <libbio/vcf/subfield/concrete_ds_access.hh>
#include <libbio/vcf/subfield/genotype_field_base_decl.hh>
#include <libbio/vcf/subfield/utility/value_access.hh>
#include <libbio/vcf/variant/sample.hh>
#include <ostream>


namespace libbio::vcf::detail {
	
	struct genotype_field_gt_helper
	{
		typedef std::vector <sample_genotype>								vector_type;
		typedef vector_value_access <sample_genotype, VCF_NUMBER_UNKNOWN>	value_access;
		
		// The same type is used for both transient and non-transient containers.
		template <bool>
		using value_access_tpl = value_access;
		
		template <bool t_is_transient>
		using base_class = subfield_concrete_ds_access <
			genotype_field_base, 
			value_access_tpl,
			t_is_transient
		>;
	};
	
	
	template <bool t_is_transient>
	class genotype_field_gt_ds_access : public detail::genotype_field_gt_helper::base_class <t_is_transient>
	{
	protected:
		typedef detail::genotype_field_gt_helper			helper_type;
		typedef helper_type::base_class <t_is_transient>	base_class;
		
	public:
		typedef genotype_field_gt_helper::vector_type		vector_type;
		typedef typename base_class::container_type			container_type;
		
	protected:
		typedef genotype_field_gt_helper::value_access		value_access;
		
	public:
		// Operator().
		vector_type &operator()(container_type &ct) const { return value_access::access_ds(this->buffer_start(ct)); }
		vector_type const &operator()(container_type const &ct) const { return value_access::access_ds(this->buffer_start(ct)); }
		
		using base_class::output_vcf_value;
		virtual void output_vcf_value(std::ostream &stream, container_type const &ct) const override { output_genotype(stream, (*this)(ct)); }
	};
}


namespace libbio::vcf {
	
	// Subclass for the GT field.
	class genotype_field_gt final :	public detail::genotype_field_gt_ds_access <true>,
									public detail::genotype_field_gt_ds_access <false>
	{
	protected:
		typedef detail::genotype_field_gt_helper::vector_type	vector_type;
		typedef detail::genotype_field_gt_helper::value_access	value_access;
			
	protected:
		virtual std::uint16_t alignment() const override { return value_access::alignment(); }
		virtual std::uint16_t byte_size() const override { return value_access::byte_size(); }
		
		virtual bool parse_and_assign(std::string_view const &sv, transient_variant_sample &sample, std::byte *mem) const override;
		
		virtual genotype_field_gt *clone() const override { return new genotype_field_gt(*this); }
		
	public:
		// As per VCF 4.3 specification Table 2.
		virtual metadata_value_type value_type() const override { return metadata_value_type::STRING; }
		virtual std::int32_t number() const override { return 1; }
	};
}

#endif

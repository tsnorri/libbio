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
#include <libbio/vcf/subfield/value_access.hh>
#include <libbio/vcf/variant/sample.hh>
#include <ostream>


namespace libbio { namespace detail {
	
	struct vcf_genotype_field_gt_helper
	{
		typedef std::vector <sample_genotype>									vector_type;
		typedef vcf_vector_value_access <sample_genotype, VCF_NUMBER_UNKNOWN>	value_access;
	};
	
	
	template <bool t_is_transient>
	struct vcf_genotype_field_gt_access_helper
	{
		typedef vcf_genotype_field_gt_helper::vector_type				vector_type;
		typedef vcf_genotype_field_gt_helper::value_access				value_access;
		
		// We use the same type for both transient and non-transient containers.
		template <bool>
		using value_access_tpl = vcf_genotype_field_gt_helper::value_access;
		
		typedef detail::vcf_subfield_concrete_ds_access <
			vcf_genotype_field_base, // not vcf_typed_genotype_field b.c. vcf_genotype_field_gt::uses_vcf_type_mapping() returns false.
			variant_sample_t,
			value_access_tpl,
			t_is_transient
		>																ds_access_base;
		
		
		class vcf_genotype_field_gt_ds_access : public ds_access_base
		{
		public:
			typedef typename ds_access_base::container_type				container_type;
			
		public:
			// Operator().
			vector_type &operator()(container_type &ct) const { return value_access::access_ds(this->buffer_start(ct)); }
			vector_type const &operator()(container_type const &ct) const { return value_access::access_ds(this->buffer_start(ct)); }
			
			using ds_access_base::output_vcf_value;
			virtual void output_vcf_value(std::ostream &stream, container_type const &ct) const override { output_genotype(stream, (*this)(ct)); }
		};
	};
	
	
	template <bool t_is_transient>
	using vcf_genotype_field_gt_access_t =
	typename vcf_genotype_field_gt_access_helper <t_is_transient>::vcf_genotype_field_gt_ds_access;
}}


namespace libbio {
	
	// Subclass for the GT field.
	// FIXME: Instead of relying on the special data member in variant_sample, vcf_genotype_field_gt could use the same buffer as the generic types.
	class vcf_genotype_field_gt final :	public detail::vcf_genotype_field_gt_access_t <true>,
		 								public detail::vcf_genotype_field_gt_access_t <false>
	{
	protected:
		typedef detail::vcf_genotype_field_gt_helper::vector_type	vector_type;
		typedef detail::vcf_genotype_field_gt_helper::value_access	value_access;
			
	protected:
		virtual std::uint16_t alignment() const override { return value_access::alignment(); }
		virtual std::uint16_t byte_size() const override { return value_access::byte_size(); }
		
		virtual bool parse_and_assign(std::string_view const &sv, transient_variant_sample &sample, std::byte *mem) const override;
		
		virtual vcf_genotype_field_gt *clone() const override { return new vcf_genotype_field_gt(*this); }
		
	public:
		// As per VCF 4.3 specification Table 2.
		virtual vcf_metadata_value_type value_type() const override { return vcf_metadata_value_type::STRING; }
		virtual std::int32_t number() const override { return 1; }
	};
}

#endif

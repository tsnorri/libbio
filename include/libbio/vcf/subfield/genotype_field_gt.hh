/*
 * Copyright (c) 2019-2020 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_SUBFIELD_GENOTYPE_FIELD_GT_HH
#define LIBBIO_VCF_SUBFIELD_GENOTYPE_FIELD_GT_HH

#include <libbio/vcf/subfield/genotype_field_base_decl.hh>
#include <libbio/vcf/variant/sample.hh>


namespace libbio {
	
	// Subclass for the GT field.
	// FIXME: Instead of relying on the special data member in variant_sample, vcf_genotype_field_gt could use the same buffer as the generic types.
	class vcf_genotype_field_gt final : public vcf_genotype_field_base // not vcf_typed_genotype_field b.c. uses_vcf_type_mapping is false.
	{
	protected:
		typedef std::vector <sample_genotype> vector_type;
			
	protected:
		virtual std::uint16_t alignment() const override { return 1; }
		virtual std::uint16_t byte_size() const override { return 0; }
		
		virtual void construct_ds(variant_sample const &, std::byte *mem, std::uint16_t const alt_count) const override { /* No-op. */ }
		virtual void construct_ds(transient_variant_sample const &, std::byte *mem, std::uint16_t const alt_count) const override { /* No-op. */ }
		virtual void destruct_ds(variant_sample const &, std::byte *mem) const override { /* No-op. */ }
		virtual void destruct_ds(transient_variant_sample const &, std::byte *mem) const override { /* No-op. */ }
		virtual void copy_ds(variant_sample const &src_var, variant_sample const &dst_var, std::byte const *src, std::byte *dst) const override { /* No-op. */ }
		virtual void copy_ds(transient_variant_sample const &src_var, variant_sample const &dst_var, std::byte const *src, std::byte *dst) const override { /* No-op. */ }
		
		virtual void reset(variant_sample const &, std::byte *mem) const override
		{
			// No-op, done in parse_and_assign.
		}
		
		virtual void reset(transient_variant_sample const &, std::byte *mem) const override
		{
			// No-op, done in parse_and_assign.
		}
		
		virtual bool parse_and_assign(std::string_view const &sv, transient_variant_sample &sample, std::byte *mem) const override;
		
		virtual vcf_genotype_field_gt *clone() const override { return new vcf_genotype_field_gt(*this); }
		
	public:
		virtual vcf_metadata_value_type value_type() const override { return vcf_metadata_value_type::STRING; }
		virtual std::int32_t number() const override { return 1; }
		virtual void output_vcf_value(std::ostream &stream, transient_variant_sample const &sample) const override;
		virtual void output_vcf_value(std::ostream &stream, variant_sample const &sample) const override;
		
		// Operator().
		vector_type &operator()(transient_variant_sample &ct) const { return ct.m_genotype; }
		vector_type const &operator()(transient_variant_sample const &ct) const { return ct.m_genotype; }
		vector_type &operator()(variant_sample &ct) const { return ct.m_genotype; }
		vector_type const &operator()(variant_sample const &ct) const { return ct.m_genotype; }
	};
}

#endif

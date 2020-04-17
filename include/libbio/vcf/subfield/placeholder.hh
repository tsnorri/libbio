/*
 * Copyright (c) 2020 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_SUBFIELD_PLACEHOLDER_HH
#define LIBBIO_VCF_SUBFIELD_PLACEHOLDER_HH

#include <libbio/vcf/subfield/genotype_field_base_decl.hh>
#include <libbio/vcf/subfield/info_field_base_decl.hh>


namespace libbio::vcf::detail {
	
	// Partial implementation.
	template <typename t_base, template <bool> typename t_container_tpl, bool t_is_transient>
	class subfield_placeholder_impl : public virtual t_base
	{
	protected:
		typedef t_container_tpl <t_is_transient>	container_type;
		typedef t_container_tpl <false>				non_transient_container_type;
		
		using t_base::reset;
		using t_base::construct_ds;
		using t_base::destruct_ds;
		using t_base::copy_ds;
		using t_base::output_vcf_value;
		
		virtual void reset(container_type const &ct, std::byte *mem) const override { /* No-op. */ }
		virtual void construct_ds(container_type const &ct, std::byte *mem, std::uint16_t const alt_count) const override { /* No-op. */ }
		virtual void destruct_ds(container_type const &ct, std::byte *mem) const override { /* No-op. */ }
		virtual void copy_ds(container_type const &src_ct,
			non_transient_container_type const &dst_ct,
			std::byte const *src,
			std::byte *dst
		) const override { /* No-op. */ }
		virtual void output_vcf_value(std::ostream &stream, container_type const &ct) const override { throw std::runtime_error("Should not be called; parse_and_assign returns false"); }
	};
}


namespace libbio::vcf {
	
	// Base classes for placeholders.
	class subfield_placeholder_base : public virtual subfield_base
	{
	public:
		virtual metadata_value_type value_type() const final { return metadata_value_type::NOT_PROCESSED; }
		
	protected:
		virtual std::uint16_t byte_size() const final { return 0; }
		virtual std::uint16_t alignment() const final { return 1; }
		virtual std::int32_t number() const final { return 0; }
	};
	
	
	class info_field_placeholder final :		public subfield_placeholder_base,
												public detail::subfield_placeholder_impl <info_field_base, variant_base_t, true>,
												public detail::subfield_placeholder_impl <info_field_base, variant_base_t, false>
	{
		virtual bool parse_and_assign(std::string_view const &sv, transient_variant &var, std::byte *mem) const { /* No-op. */ return false; }
		virtual info_field_placeholder *clone() const { return new info_field_placeholder(*this); }
	};
	
	
	class genotype_field_placeholder final :	public subfield_placeholder_base,
												public detail::subfield_placeholder_impl <genotype_field_base, variant_sample_t, true>,
												public detail::subfield_placeholder_impl <genotype_field_base, variant_sample_t, false>
	{
		virtual bool parse_and_assign(std::string_view const &sv, transient_variant_sample &sample, std::byte *mem) const { /* No-op. */ return false; }
		virtual genotype_field_placeholder *clone() const { return new genotype_field_placeholder(*this); }
	};
}

#endif

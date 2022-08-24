/*
 * Copyright (c) 2020 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_SUBFIELD_PLACEHOLDER_HH
#define LIBBIO_VCF_SUBFIELD_PLACEHOLDER_HH

#include <libbio/vcf/subfield/genotype_field_base_decl.hh>
#include <libbio/vcf/subfield/info_field_base_decl.hh>


namespace libbio::vcf::detail {
	
	template <typename t_base>
	class subfield_placeholder_base : public t_base
	{
	protected:
		typedef typename t_base::container_type				container_type;
		typedef typename t_base::transient_container_type	transient_container_type;
		
	public:
		enum metadata_value_type metadata_value_type() const override { return metadata_value_type::NOT_PROCESSED; }
		
	protected:
		std::uint16_t byte_size() const override { return 0; }
		std::uint16_t alignment() const override { return 1; }
		std::int32_t number() const override { return 0; }

		void reset(container_type const &ct, std::byte *mem) const override { /* No-op. */ }
		void reset(transient_container_type const &ct, std::byte *mem) const override { /* No-op. */ }
		void construct_ds(container_type const &ct, std::byte *mem, std::uint16_t const alt_count) const override { /* No-op. */ }
		void construct_ds(transient_container_type const &ct, std::byte *mem, std::uint16_t const alt_count) const override { /* No-op. */ }
		void destruct_ds(container_type const &ct, std::byte *mem) const override { /* No-op. */ }
		void destruct_ds(transient_container_type const &ct, std::byte *mem) const override { /* No-op. */ }
		void copy_ds(transient_container_type const &src_ct,
			container_type const &dst_ct,
			std::byte const *src,
			std::byte *dst
		) const override { /* No-op. */ }
		void copy_ds(container_type const &src_ct,
			container_type const &dst_ct,
			std::byte const *src,
			std::byte *dst
		) const override { /* No-op. */ }
		
		void output_vcf_value(std::ostream &stream, container_type const &ct) const override { throw std::runtime_error("Should not be called; parse_and_assign returns false"); }
		void output_vcf_value(std::ostream &stream, transient_container_type const &ct) const override { throw std::runtime_error("Should not be called; parse_and_assign returns false"); }
	};
}


namespace libbio::vcf {
	
	class info_field_placeholder final : public detail::subfield_placeholder_base <info_field_base>
	{
		bool parse_and_assign(std::string_view const &sv, transient_variant &var, std::byte *mem) const override { /* No-op. */ return false; }
		info_field_placeholder *clone() const override { return new info_field_placeholder(*this); }
	};
	
	
	class genotype_field_placeholder final : public detail::subfield_placeholder_base <genotype_field_base>
	{
		bool parse_and_assign(std::string_view const &sv, transient_variant const &var, transient_variant_sample &sample, std::byte *mem) const override { /* No-op. */ return false; }
		genotype_field_placeholder *clone() const override { return new genotype_field_placeholder(*this); }
	};
}

#endif

/*
 * Copyright (c) 2020 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_SUBFIELD_CONCRETE_DS_ACCESS_HH
#define LIBBIO_VCF_SUBFIELD_CONCRETE_DS_ACCESS_HH

#include <libbio/assert.hh>
#include <libbio/vcf/subfield/base.hh>
#include <libbio/vcf/subfield/copy_value.hh>
#include <ostream>


namespace libbio {
	
	class vcf_info_field_base;
	class vcf_genotype_field_base;
}


namespace libbio { namespace detail {
	
	// Generic implementation of accessors for one container type.
	// m_metadata located in vcf_*_field_base.
	template <
		typename t_base,
		template <bool> typename t_container_tpl,
		template <bool> typename t_access_tpl,
		bool t_is_transient
	>
	class vcf_subfield_concrete_ds_access : public virtual t_base
	{
		//static_assert(std::is_same_v <t_base, vcf_info_field_base> || std::is_same_v <t_base, vcf_genotype_field_base>);
		
	protected:
		typedef t_container_tpl <t_is_transient>	container_type;
		typedef t_container_tpl <false>				non_transient_container_type;
		typedef t_access_tpl <t_is_transient>		access_type;
		typedef t_access_tpl <false>				non_transient_access_type;
	
	protected:
		using t_base::construct_ds;
		using t_base::destruct_ds;
		using t_base::copy_ds;
		using t_base::reset;
	
		// Construct the type in the memory block. The additional parameters are passed to the constructor if needed.
		virtual void construct_ds(container_type const &ct, std::byte *mem, std::uint16_t const alt_count) const override final
		{
			libbio_always_assert_neq(vcf_subfield_base::INVALID_OFFSET, this->m_offset);
			access_type::construct_ds(mem + this->m_offset, alt_count, *this->m_metadata);
		}
	
		// Destruct the type in the memory block (if needed).
		virtual void destruct_ds(container_type const &ct, std::byte *mem) const override final
		{
			libbio_always_assert_neq(vcf_subfield_base::INVALID_OFFSET, this->m_offset);
			access_type::destruct_ds(mem + this->m_offset);
		}
	
		// Copy the data structures.
		virtual void copy_ds(
			container_type const &src_ct,
			non_transient_container_type const &dst_ct,
			std::byte const *src,
			std::byte *dst
		) const override final
		{
			libbio_always_assert_neq(vcf_subfield_base::INVALID_OFFSET, this->m_offset);
			libbio_assert(src);
			libbio_assert(dst);
			auto const &srcv(access_type::access_ds(src + this->m_offset));
			auto &dstv(non_transient_access_type::access_ds(dst + this->m_offset));
			detail::copy_value(srcv, dstv);
		}
	
		virtual void reset(container_type const &ct, std::byte *mem) const override final
		{
			libbio_always_assert_neq(vcf_subfield_base::INVALID_OFFSET, this->m_offset);
			access_type::reset_ds(mem + this->m_offset);
		}
	};
}}

#endif

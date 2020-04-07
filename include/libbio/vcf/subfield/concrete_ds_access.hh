/*
 * Copyright (c) 2020 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_SUBFIELD_CONCRETE_DS_ACCESS_HH
#define LIBBIO_VCF_SUBFIELD_CONCRETE_DS_ACCESS_HH

#include <libbio/vcf/subfield/copy_value.hh>
#include <libbio/vcf/subfield/storable_subfield_base.hh>
#include <ostream>


namespace libbio { namespace detail {
	
	// Generic implementation of accessors for one container type.
	// construct_ds etc. defined in vcf_subfield_ds_access
	//	inherited by vcf_*_field_base
	//		virtually inherited by vcf_typed_field
	//		virtually inherited by vcf_storable_*_field_base
	//			inherited by t_base.
	// m_metadata and m_offset located in vcf_*_field_base and vcf_storable_subfield_base respectively.
	template <
		typename t_base,
		template <bool> typename t_container_tpl,
		template <bool> typename t_access_tpl,
		bool t_is_transient
	>
	class vcf_subfield_concrete_ds_access : public virtual t_base
	{
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
		virtual void construct_ds(container_type const &ct, std::byte *mem, std::uint16_t const alt_count) const override
		{
			libbio_always_assert_neq(vcf_storable_subfield_base::INVALID_OFFSET, this->m_offset);
			access_type::construct_ds(mem + this->m_offset, alt_count, *this->m_metadata);
		}
	
		// Destruct the type in the memory block (if needed).
		virtual void destruct_ds(container_type const &ct, std::byte *mem) const override
		{
			libbio_always_assert_neq(vcf_storable_subfield_base::INVALID_OFFSET, this->m_offset);
			access_type::destruct_ds(mem + this->m_offset);
		}
	
		// Copy the data structures.
		virtual void copy_ds(
			container_type const &src_ct,
			non_transient_container_type const &dst_ct,
			std::byte const *src,
			std::byte *dst
		) const override
		{
			libbio_always_assert_neq(vcf_storable_subfield_base::INVALID_OFFSET, this->m_offset);
			libbio_assert(src);
			libbio_assert(dst);
			auto const &srcv(access_type::access_ds(src + this->m_offset));
			auto &dstv(non_transient_access_type::access_ds(dst + this->m_offset));
			detail::copy_value(srcv, dstv);
		}
	
		virtual void reset(container_type const &ct, std::byte *mem) const override
		{
			libbio_always_assert_neq(vcf_storable_subfield_base::INVALID_OFFSET, this->m_offset);
			access_type::reset_ds(mem + this->m_offset);
		}
		
	public:
		using t_base::output_vcf_value;
		
		virtual void output_vcf_value(std::ostream &stream, container_type const &ct) const override
		{
			libbio_always_assert_neq(vcf_storable_subfield_base::INVALID_OFFSET, this->m_offset);
			access_type::output_vcf_value(stream, this->buffer_start(ct) + this->m_offset);
		}
	};
}}

#endif

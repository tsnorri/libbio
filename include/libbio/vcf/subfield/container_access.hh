/*
 * Copyright (c) 2020-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_SUBFIELD_CONTAINER_ACCESS_HH
#define LIBBIO_VCF_SUBFIELD_CONTAINER_ACCESS_HH

#include <libbio/assert.hh>
#include <libbio/vcf/subfield/base.hh>
#include <libbio/vcf/subfield/utility/copy_value.hh>


namespace libbio::vcf {

	class info_field_base;
	class genotype_field_base;
}


namespace libbio::vcf::detail {

#if 0
	// Generic implementation of accessors for one container type.
	// m_metadata located in *_field_base.
	template <
		template <bool> typename t_access_tpl,
		bool t_is_transient
	>
	class subfield_container_access
	{
	protected:
		template <bool B>
		using container_tpl = typename t_base::template container_tpl <B>;

		typedef container_tpl <t_is_transient>	container_type;
		typedef container_tpl <false>			non_transient_container_type;
		typedef t_access_tpl <t_is_transient>	access_type;
		typedef t_access_tpl <false>			non_transient_access_type;

	protected:
		// Construct the type in the memory block. The additional parameters are passed to the constructor if needed.
		virtual void construct_ds(container_type const &ct, std::byte *mem, std::uint16_t const alt_count) const override final
		{
			libbio_always_assert_neq(subfield_base::INVALID_OFFSET, this->m_offset);
			access_type::construct_ds(mem + this->m_offset, alt_count, *this->m_metadata);
		}

		// Destruct the type in the memory block (if needed).
		virtual void destruct_ds(container_type const &ct, std::byte *mem) const override final
		{
			libbio_always_assert_neq(subfield_base::INVALID_OFFSET, this->m_offset);
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
			libbio_always_assert_neq(subfield_base::INVALID_OFFSET, this->m_offset);
			libbio_assert(src);
			libbio_assert(dst);
			auto const &srcv(access_type::access_ds(src + this->m_offset));
			auto &dstv(non_transient_access_type::access_ds(dst + this->m_offset));
			detail::copy_value(srcv, dstv);
		}

		virtual void reset(container_type const &ct, std::byte *mem) const
		{
			libbio_always_assert_neq(subfield_base::INVALID_OFFSET, this->m_offset);
			access_type::reset_ds(mem + this->m_offset);
		}
	};
#endif
}

#endif

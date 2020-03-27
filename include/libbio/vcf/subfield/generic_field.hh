/*
 * Copyright (c) 2020 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_SUBFIELD_GENERIC_FIELD_HH
#define LIBBIO_VCF_SUBFIELD_GENERIC_FIELD_HH

#include <libbio/vcf/subfield/access.hh>
#include <libbio/vcf/subfield/decl.hh>
#include <libbio/vcf/subfield/parser.hh>
#include <libbio/vcf/subfield/storable_subfield_base.hh>
#include <libbio/vcf/subfield/typed_field.hh>
#include <libbio/vcf/variant/abstract_variant_decl.hh>
#include <libbio/vcf/variant/sample.hh>


namespace libbio { namespace detail {
	
	// The following are implemented in vcf_generic_field_ds_access (for both values of t_is_transient):
	// operator() defined in vcf_typed_field.
	// construct_ds etc. defined in vcf_subfield_ds_access
	//	inherited by vcf_*_field_base
	//		virtually inherited by vcf_typed_field
	//		virtually inherited by vcf_storable_*_field_base
	//			inherited by t_base.
	// m_metadata and m_offset located in vcf_*_field_base and vcf_storable_subfield_base respectively.
	
	template <
		std::int32_t t_number,
		vcf_metadata_value_type t_value_type,
		template <std::int32_t, vcf_metadata_value_type> class t_base
	>
	using vcf_generic_field_ds_access_typed_field_base_t = vcf_typed_field <
		t_value_type,
		vcf_value_count_corresponds_to_vector(t_number),
		t_base <t_number, t_value_type>::template container_tpl,
		typename t_base <t_number, t_value_type>::virtual_base
	>;
	
	template <
		std::int32_t t_number,
		vcf_metadata_value_type t_value_type,
		template <std::int32_t, vcf_metadata_value_type> class t_base,
		bool t_is_transient
	>
	class vcf_generic_field_ds_access :	public virtual t_base <t_number, t_value_type>,
										public virtual vcf_generic_field_ds_access_typed_field_base_t <
											t_number, t_value_type, t_base
										>
	{
	protected:
		typedef t_base <t_number, t_value_type>									generic_field_base;
		typedef vcf_generic_field_ds_access_typed_field_base_t <
			t_number, t_value_type, t_base
		>																		typed_field_base;
		
		template <bool B> using container_tpl = typename generic_field_base::template container_tpl <B>;
		
		typedef container_tpl <t_is_transient>									container_type;
		typedef container_tpl <false>											non_transient_container_type;
		typedef vcf_subfield_access <t_number, t_value_type, t_is_transient>	field_access;
		typedef vcf_value_type_mapping_t <
			t_value_type,
			vcf_value_count_corresponds_to_vector(t_number),
			t_is_transient
		>																		value_type;
		
	protected:
		using generic_field_base::construct_ds;
		using generic_field_base::destruct_ds;
		using generic_field_base::copy_ds;
		using generic_field_base::reset;
		
		// Construct the type in the memory block. The additional parameters are passed to the constructor if needed.
		virtual void construct_ds(container_type const &ct, std::byte *mem, std::uint16_t const alt_count) const override
		{
			libbio_always_assert_neq(vcf_storable_subfield_base::INVALID_OFFSET, this->m_offset);
			field_access::construct_ds(mem + this->m_offset, alt_count, *this->m_metadata);
		}
		
		// Destruct the type in the memory block (if needed).
		virtual void destruct_ds(container_type const &ct, std::byte *mem) const override
		{
			libbio_always_assert_neq(vcf_storable_subfield_base::INVALID_OFFSET, this->m_offset);
			field_access::destruct_ds(mem + this->m_offset);
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
			field_access::copy_ds(src + this->m_offset, dst + this->m_offset);
		}
		
		virtual void reset(container_type const &ct, std::byte *mem) const override
		{
			libbio_always_assert_neq(vcf_storable_subfield_base::INVALID_OFFSET, this->m_offset);
			field_access::reset_ds(mem + this->m_offset);
		}
		
	public:
		using generic_field_base::output_vcf_value;
		using typed_field_base::operator();
		
		virtual void output_vcf_value(std::ostream &stream, container_type const &ct) const override
		{
			libbio_always_assert_neq(vcf_storable_subfield_base::INVALID_OFFSET, this->m_offset);
			field_access::output_vcf_value(stream, this->buffer_start(ct) + this->m_offset);
		}
		
		// Operator().
		virtual value_type &operator()(container_type &ct) const override
		{
			return field_access::access_ds(this->buffer_start(ct));
		}

		virtual value_type const &operator()(container_type const &ct) const override
		{
			return field_access::access_ds(this->buffer_start(ct));
		}
	};
}}


namespace libbio {
	
	// Base classes for the generic field types, implement byte_size().
	template <std::int32_t t_number, vcf_metadata_value_type t_value_type>
	class vcf_generic_info_field_base :
		public vcf_storable_info_field_base,
		public vcf_generic_field_parser <t_number, t_value_type>
	{
	public:
		template <bool t_transient> using container_tpl	= variant_base_t <t_transient>;
		typedef vcf_info_field_base						virtual_base;
		
	protected:
		typedef vcf_subfield_access <t_number, t_value_type, true>	field_access;
		typedef vcf_generic_field_parser <t_number, t_value_type>	parser_type;
		
	protected:
		// Access the container’s buffer, for use with operator().
		std::byte *buffer_start(abstract_variant const &ct) const { return ct.m_info.get(); }
		
		// Assign a value, used for FLAG.
		virtual bool assign(std::byte *mem) const override final
		{
			libbio_always_assert_neq(vcf_storable_subfield_base::INVALID_OFFSET, m_offset);
			field_access::add_value(mem + m_offset, 0);
			return true;
		};
		
		virtual bool parse_and_assign(std::string_view const &sv, transient_variant &var, std::byte *mem) const override final
		{
			libbio_always_assert_neq(vcf_storable_subfield_base::INVALID_OFFSET, m_offset);
			parser_type::parse_and_assign(sv, mem + m_offset);
			return true;
		}
	};
	
	
	template <std::int32_t t_number, vcf_metadata_value_type t_value_type>
	class vcf_generic_genotype_field_base :
		public vcf_storable_genotype_field_base,
		public vcf_generic_field_parser <t_number, t_value_type>
	{
	public:
		template <bool t_transient> using container_tpl	= variant_sample_t <t_transient>;
		typedef vcf_genotype_field_base								virtual_base;
		
	protected:
		typedef vcf_generic_field_parser <t_number, t_value_type>	parser_type;
		
	protected:
		// Access the container’s buffer, for use with operator().
		std::byte *buffer_start(variant_sample_base const &vs) const { return vs.m_sample_data.get(); }
		
		virtual bool parse_and_assign(std::string_view const &sv, transient_variant_sample &var, std::byte *mem) const override final
		{
			libbio_always_assert_neq(vcf_storable_subfield_base::INVALID_OFFSET, m_offset);
			return parser_type::parse_and_assign(sv, mem + m_offset);
		}
	};
	
	
	// A generic field type.
	template <
		std::int32_t t_number,
		vcf_metadata_value_type t_value_type,
		template <std::int32_t, vcf_metadata_value_type> class t_base
	>
	class vcf_generic_field final :
		public detail::vcf_generic_field_ds_access <t_number, t_value_type, t_base, true>,
		public detail::vcf_generic_field_ds_access <t_number, t_value_type, t_base, false>
	{
	protected:
		template <bool t_is_transient>
		using field_access = vcf_subfield_access <t_number, t_value_type, t_is_transient>;
		
	protected:
		// Assume that the alignment and the size are close enough for both types.
		// FIXME: alignment() and byte_size() could be made final in typed_field b.c. the value type is already known there.
		constexpr virtual std::uint16_t alignment() const override
		{
			return std::max(field_access <true>::alignment(), field_access <false>::alignment());
		}
		
		constexpr virtual std::uint16_t byte_size() const override
		{
			return std::max(field_access <true>::byte_size(), field_access <false>::byte_size());
		}
		
		virtual vcf_generic_field *clone() const override { return new vcf_generic_field(*this); }
		
	public:
		constexpr virtual std::int32_t number() const override { return t_number; }
	};
}

#endif

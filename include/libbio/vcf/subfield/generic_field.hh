/*
 * Copyright (c) 2020 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_SUBFIELD_GENERIC_FIELD_HH
#define LIBBIO_VCF_SUBFIELD_GENERIC_FIELD_HH

#include <libbio/vcf/subfield/access.hh>
#include <libbio/vcf/subfield/base.hh>
#include <libbio/vcf/subfield/concrete_ds_access.hh>
#include <libbio/vcf/subfield/decl.hh>
#include <libbio/vcf/subfield/parser.hh>
#include <libbio/vcf/subfield/typed_field.hh>


namespace libbio { namespace detail {
	
	template <typename t_base>
	struct vcf_generic_field_ds_access_helper
	{
		template <bool B>
		using field_access_tpl = vcf_subfield_access <t_base::s_number(), t_base::s_value_type(), B>;
	};
	
	
	// t_base is one of vcf_generic_info_field_base, vcf_generic_genotype_field_base.
	// operator() defined in vcf_typed_field.
	template <typename t_base, bool t_is_transient>
	class vcf_generic_field_ds_access :	public virtual t_base::typed_field_base,
										public vcf_subfield_concrete_ds_access <
											typename t_base::typed_field_base::virtual_base,
											t_base::template container_tpl,
											vcf_generic_field_ds_access_helper <t_base>::template field_access_tpl,
											t_is_transient
										>
	{
	protected:
		typedef typename t_base::typed_field_base							typed_field_base;
		
		typedef vcf_value_type_mapping_t <
			t_base::s_value_type(),
			vcf_value_count_corresponds_to_vector(t_base::s_number()),
			t_is_transient
		>																	value_type;
		
		template <bool B>
		using container_tpl = typename t_base::template container_tpl <B>;
		
		template <bool B>
		using field_access_tpl = typename vcf_generic_field_ds_access_helper <t_base>::template field_access_tpl <B>;
		
		typedef container_tpl <t_is_transient>								container_type;
		typedef container_tpl <false>										non_transient_container_type;
		typedef field_access_tpl <t_is_transient>							field_access;
		typedef field_access_tpl <false>									non_transient_field_access;
		
	public:
		using typed_field_base::operator();
		using typed_field_base::output_vcf_value;
	
		virtual void output_vcf_value(std::ostream &stream, container_type const &ct) const override final
		{
			libbio_always_assert_neq(vcf_subfield_base::INVALID_OFFSET, this->m_offset);
			field_access::output_vcf_value(stream, this->buffer_start(ct) + this->m_offset);
		}
		
		// Operator().
		virtual value_type &operator()(container_type &ct) const override final
		{
			return field_access::access_ds(this->buffer_start(ct));
		}
		
		virtual value_type const &operator()(container_type const &ct) const override final
		{
			return field_access::access_ds(this->buffer_start(ct));
		}
	};
}}


namespace libbio {
	
	// Base classes for the generic field types.
	template <std::int32_t t_number, vcf_metadata_value_type t_value_type>
	class vcf_generic_info_field_base :
		public virtual vcf_typed_info_field_t <vcf_value_count_corresponds_to_vector(t_number), t_value_type>,
		public vcf_generic_field_parser <t_number, t_value_type>
	{
	public:
		template <bool t_transient>
		using container_tpl	= variant_base_t <t_transient>;
		
		typedef vcf_typed_info_field_t <
			vcf_value_count_corresponds_to_vector(t_number),
			t_value_type
		>															typed_field_base;
		
	protected:
		typedef vcf_subfield_access <t_number, t_value_type, true>	field_access; // May be transient b.c. field_access is only used for parse_and_assign and assign here.
		typedef vcf_generic_field_parser <t_number, t_value_type>	parser_type;
		
	public:
		static constexpr std::int32_t s_number() { return t_number; }
		static constexpr vcf_metadata_value_type s_value_type() { return t_value_type; }
		
	protected:
		// Assign a value, used for FLAG.
		virtual bool assign(std::byte *mem) const override final
		{
			libbio_always_assert_neq(vcf_subfield_base::INVALID_OFFSET, this->m_offset);
			field_access::add_value(mem + this->m_offset, 0);
			return true;
		};
		
		virtual bool parse_and_assign(std::string_view const &sv, transient_variant &var, std::byte *mem) const override final
		{
			libbio_always_assert_neq(vcf_subfield_base::INVALID_OFFSET, this->m_offset);
			parser_type::parse_and_assign(sv, mem + this->m_offset);
			return true;
		}
	};
	
	
	template <std::int32_t t_number, vcf_metadata_value_type t_value_type>
	class vcf_generic_genotype_field_base :
		public virtual vcf_typed_genotype_field_t <vcf_value_count_corresponds_to_vector(t_number), t_value_type>,
		public vcf_generic_field_parser <t_number, t_value_type>
	{
	public:
		template <bool t_transient>
		using container_tpl	= variant_sample_t <t_transient>;
		
		typedef vcf_typed_genotype_field_t <
			vcf_value_count_corresponds_to_vector(t_number),
			t_value_type
		>															typed_field_base;
		
	protected:
		typedef vcf_generic_field_parser <t_number, t_value_type>	parser_type;
		
	public:
		static constexpr std::int32_t s_number() { return t_number; }
		static constexpr vcf_metadata_value_type s_value_type() { return t_value_type; }
		
	protected:
		virtual bool parse_and_assign(std::string_view const &sv, transient_variant_sample &var, std::byte *mem) const override final
		{
			libbio_always_assert_neq(vcf_subfield_base::INVALID_OFFSET, this->m_offset);
			return parser_type::parse_and_assign(sv, mem + this->m_offset);
		}
	};
	
	
	// A generic field type.
	// t_base is one of vcf_generic_info_field_base, vcf_generic_genotype_field_base.
	template <typename t_base>
	class vcf_generic_field final :
		public t_base,
		public detail::vcf_generic_field_ds_access <t_base, true>,
		public detail::vcf_generic_field_ds_access <t_base, false>
	{
		static_assert(
			std::is_same_v <
				t_base,
				vcf_generic_info_field_base <t_base::s_number(), t_base::s_value_type()>
			> ||
			std::is_same_v <
				t_base,
				vcf_generic_genotype_field_base <t_base::s_number(), t_base::s_value_type()>
			>
		);
		
	protected:
		template <bool t_is_transient>
		using field_access = vcf_subfield_access <t_base::s_number(), t_base::s_value_type(), t_is_transient>;
		
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
		constexpr virtual std::int32_t number() const override { return t_base::s_number(); }
	};
}

#endif

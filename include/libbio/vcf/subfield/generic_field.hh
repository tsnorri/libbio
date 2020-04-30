/*
 * Copyright (c) 2020 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_SUBFIELD_GENERIC_FIELD_HH
#define LIBBIO_VCF_SUBFIELD_GENERIC_FIELD_HH

#include <libbio/vcf/subfield/base.hh>
#include <libbio/vcf/subfield/container_access.hh>
#include <libbio/vcf/subfield/decl.hh>
#include <libbio/vcf/subfield/typed_field.hh>
#include <libbio/vcf/subfield/utility/access.hh>
#include <libbio/vcf/subfield/utility/parser.hh>


namespace libbio::vcf::detail {
	
	// Handle the necessary checks and pass calls to the actual field_access class.
	template <typename t_field, bool t_is_transient>
	struct generic_field_access
	{
		typedef typename t_field::template field_access_tpl <t_is_transient>	access_type;
		typedef typename t_field::template field_access_tpl <false>				non_transient_access_type;
		typedef typename t_field::template container_tpl <t_is_transient>		container_type; // Transient or not.
		typedef typename t_field::container_type								non_transient_container_type;
		typedef typename access_type::value_type								value_type;
		
		// FIXME: why do these member functions take a memory address but access_ds and output_vcf_value do not?
		static void construct_ds(t_field const &field, container_type const &ct, std::byte *mem, std::uint16_t const alt_count)
		{
			libbio_always_assert_neq(subfield_base::INVALID_OFFSET, field.m_offset);
			access_type::construct_ds(mem + field.m_offset, alt_count, *field.m_metadata);
		}
		
		static void destruct_ds(t_field const &field, container_type const &ct, std::byte *mem)
		{
			libbio_always_assert_neq(subfield_base::INVALID_OFFSET, field.m_offset);
			access_type::destruct_ds(mem + field.m_offset);
		}
		
		static void copy_ds(
			t_field const &field,
			container_type const &src_ct,
			non_transient_container_type const &dst_ct,
			std::byte const *src,
			std::byte *dst
		)
		{
			libbio_always_assert_neq(subfield_base::INVALID_OFFSET, field.m_offset);
			libbio_assert(src);
			libbio_assert(dst);
			auto const &srcv(access_type::access_ds(src + field.m_offset));
			auto &dstv(non_transient_access_type::access_ds(dst + field.m_offset));
			detail::copy_value(srcv, dstv);
		}
		
		static void reset(t_field const &field, container_type const &ct, std::byte *mem)
		{
			libbio_always_assert_neq(subfield_base::INVALID_OFFSET, field.m_offset);
			access_type::reset_ds(mem + field.m_offset);
		}
		
		static value_type const &access_ds(t_field const &field, container_type const &ct)
		{
			auto const &val(access_type::access_ds(field.buffer_start(ct) + field.m_offset));
			return val;
		}
		
		static value_type &access_ds(t_field const &field, container_type &ct)
		{
			auto &val(access_type::access_ds(field.buffer_start(ct) + field.m_offset));
			return val;
		}
		
		static void output_vcf_value(t_field const &field, std::ostream &stream, container_type const &ct)
		{
			libbio_always_assert_neq(subfield_base::INVALID_OFFSET, field.m_offset);
			access_type::output_vcf_value(stream, field.buffer_start(ct) + field.m_offset);
		}
	};
	
	
	// t_base is e.g. type_mapped_field_base.
	template <typename t_base>
	class generic_field_tpl : public t_base
	{
		template <typename, bool>
		friend struct generic_field_access;
		
	protected:
		template <bool B>
		using field_access_tpl = typename t_base::template field_access_tpl <B>;
		
		template <bool B>
		using container_tpl = typename t_base::template container_tpl <B>;
		
	public:
		typedef container_tpl <false>							container_type;
		typedef container_tpl <true>							transient_container_type;
		typedef field_access_tpl <false>						field_access;
		typedef field_access_tpl <true>							transient_field_access;
		typedef typename field_access::value_type				value_type;
		typedef typename transient_field_access::value_type		transient_value_type;
		
	protected:
		typedef generic_field_access <generic_field_tpl, false>	access_wrapper;
		typedef generic_field_access <generic_field_tpl, true>	transient_access_wrapper;
		
	public:
		using t_base::output_vcf_value;
		
		virtual void output_vcf_value(std::ostream &stream, container_type const &ct) const override
		{
			access_wrapper::output_vcf_value(*this, stream, ct);
		}
		
		virtual void output_vcf_value(std::ostream &stream, transient_container_type const &ct) const override
		{
			transient_access_wrapper::output_vcf_value(*this, stream, ct);
		}
		
		// Operator().
		// If typed_field is a base class, these actually override operator() but not otherwise.
		// To solve this, we abuse the final keyword b.c. as it implies override.
		virtual value_type &operator()(container_type &ct) const final
		{
			auto &val(access_wrapper::access_ds(*this, ct));
			return val;
		}
		
		virtual value_type const &operator()(container_type const &ct) const final
		{
			auto const &val(access_wrapper::access_ds(*this, ct));
			return val;
		}
		
		virtual transient_value_type &operator()(transient_container_type &ct) const final
		{
			auto &val(transient_access_wrapper::access_ds(*this, ct));
			return val;
		}
		
		virtual transient_value_type const &operator()(transient_container_type const &ct) const final
		{
			auto const &val(transient_access_wrapper::access_ds(*this, ct));
			return val;
		}
		
	protected:
		// Assume that the alignment and the size are close enough for both types.
		// FIXME: alignment() and byte_size() could be made final in typed_field b.c. the value type is already known there.
		constexpr virtual std::uint16_t alignment() const override
		{
			return std::max(field_access::alignment(), transient_field_access::alignment());
		}
		
		constexpr virtual std::uint16_t byte_size() const override
		{
			return std::max(field_access::byte_size(), transient_field_access::byte_size());
		}
		
		virtual void reset(container_type const &ct, std::byte *mem) const override
		{
			access_wrapper::reset(*this, ct, mem);
		}
		
		virtual void reset(transient_container_type const &ct, std::byte *mem) const override
		{
			transient_access_wrapper::reset(*this, ct, mem);
		}
	
		// Construct the data structure.
		virtual void construct_ds(container_type const &ct, std::byte *mem, std::uint16_t const alt_count) const override
		{
			access_wrapper::construct_ds(*this, ct, mem, alt_count);
		}
		
		virtual void construct_ds(transient_container_type const &ct, std::byte *mem, std::uint16_t const alt_count) const override
		{
			transient_access_wrapper::construct_ds(*this, ct, mem, alt_count);
		}
	
		// Destruct the datastructure.
		virtual void destruct_ds(container_type const &ct, std::byte *mem) const override
		{
			access_wrapper::destruct_ds(*this, ct, mem);
		}
		
		virtual void destruct_ds(transient_container_type const &ct, std::byte *mem) const override
		{
			transient_access_wrapper::destruct_ds(*this, ct, mem);
		}
	
		// Copy the data structure.
		virtual void copy_ds(
			container_type const &src_ct,
			container_type const &dst_ct,
			std::byte const *src,
			std::byte *dst
		) const override
		{
			access_wrapper::copy_ds(*this, src_ct, dst_ct, src, dst);
		}
	
		virtual void copy_ds(
			transient_container_type const &src_ct,
			container_type const &dst_ct,
			std::byte const *src,
			std::byte *dst
		) const override
		{
			transient_access_wrapper::copy_ds(*this, src_ct, dst_ct, src, dst);
		}
	};
	
	
	// Base class for generic_field_tpl if type mapping is used.
	template <
		metadata_value_type t_value_type,
		std::int32_t t_number,
		template <metadata_value_type, bool> typename typed_base_tpl
	>
	struct type_mapped_field_base :	public typed_base_tpl <
										t_value_type,
										value_count_corresponds_to_vector(t_number)
									>,
									public generic_field_parser <t_value_type, t_number> // Always uses transient value type.
	{
	protected:
		template <bool B>
		using field_access_tpl = subfield_access <t_value_type, t_number, B>;
		
		typedef field_access_tpl <false>						value_type;
		typedef field_access_tpl <true>							transient_value_type;
		
		typedef generic_field_parser <t_value_type, t_number>	parser_type;
	};
	
	
	template <
		metadata_value_type t_value_type,
		std::int32_t t_number,
		template <metadata_value_type, bool> typename t_typed_base_tpl
	>
	using generic_typed_field_t = generic_field_tpl <
		detail::type_mapped_field_base <
			t_value_type,
			t_number,
			t_typed_base_tpl
		>
	>;
}


namespace libbio::vcf {
	
	// Base classes for the generic field types.
	template <metadata_value_type t_value_type, std::int32_t t_number>
	class generic_info_field_base : public detail::generic_typed_field_t <t_value_type, t_number, typed_info_field_t>
	{
	public:
		typedef detail::generic_typed_field_t <
			t_value_type,
			t_number,
			typed_info_field_t
		>																base_class;
		
		typedef typename base_class::container_type						container_type;
		typedef typename base_class::transient_container_type			transient_container_type;
		typedef typename base_class::value_type							value_type;
		typedef typename base_class::transient_value_type				transient_value_type;
		
	protected:
		typedef typename base_class::template field_access_tpl <true>	transient_field_access; // May be transient b.c. only used for assign here.
		typedef typename base_class::parser_type						parser_type;
		
	public:
		static constexpr std::int32_t s_number() { return t_number; }
		static constexpr metadata_value_type s_value_type() { return t_value_type; }
		
	protected:
		// Assign a value, used for FLAG.
		virtual bool assign(std::byte *mem) const override
		{
			libbio_always_assert_neq(subfield_base::INVALID_OFFSET, this->m_offset);
			transient_field_access::add_value(mem + this->m_offset, 0);
			return true; // FIXME: return parse_and_assign’s return value?
		};
		
		virtual bool parse_and_assign(std::string_view const &sv, transient_variant &var, std::byte *mem) const override
		{
			libbio_always_assert_neq(subfield_base::INVALID_OFFSET, this->m_offset);
			parser_type::parse_and_assign(sv, mem + this->m_offset);
			return true; // FIXME: return parse_and_assign’s return value?
		}
	};
	
	
	template <metadata_value_type t_value_type, std::int32_t t_number>
	class generic_genotype_field_base : public detail::generic_typed_field_t <t_value_type, t_number, typed_genotype_field_t>
	{
	public:
		typedef detail::generic_typed_field_t <
			t_value_type,
			t_number,
			typed_genotype_field_t
		>														base_class;
		
		typedef typename base_class::container_type				container_type;
		typedef typename base_class::transient_container_type	transient_container_type;
		typedef typename base_class::value_type					value_type;
		typedef typename base_class::transient_value_type		transient_value_type;
		
	protected:
		typedef typename base_class::parser_type				parser_type;
		
	public:
		static constexpr std::int32_t s_number() { return t_number; }
		static constexpr metadata_value_type s_value_type() { return t_value_type; }
		
	protected:
		virtual bool parse_and_assign(std::string_view const &sv, transient_variant_sample &var, std::byte *mem) const override final
		{
			libbio_always_assert_neq(subfield_base::INVALID_OFFSET, this->m_offset);
			return parser_type::parse_and_assign(sv, mem + this->m_offset);
		}
	};
	
	
	// A generic field type.
	// t_base is one of generic_info_field_base, generic_genotype_field_base.
	template <typename t_base>
	class generic_field final : public t_base
	{
		typedef generic_info_field_base <t_base::s_value_type(), t_base::s_number()>		info_field_base;
		typedef generic_genotype_field_base <t_base::s_value_type(), t_base::s_number()>	genotype_field_base;
		static_assert(
			std::is_same_v <t_base, info_field_base> ||
			std::is_same_v <t_base, genotype_field_base>
		);
		
	public:
		typedef typename t_base::container_type				container_type;
		typedef typename t_base::transient_container_type	transient_container_type;
		typedef typename t_base::value_type					value_type;
		typedef typename t_base::transient_value_type		transient_value_type;
		
	protected:
		virtual generic_field *clone() const override { return new generic_field(*this); }
		
	public:
		constexpr virtual std::int32_t number() const override { return t_base::s_number(); }
	};
}

#endif

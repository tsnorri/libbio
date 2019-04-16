/*
 * Copyright (c) 2019 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_SUBFIELD_DEF_HH
#define LIBBIO_VCF_SUBFIELD_DEF_HH

#include <libbio/assert.hh>
#include <libbio/vcf/variant.hh>
#include <libbio/vcf/vcf_subfield.hh>
#include <range/v3/algorithm/copy.hpp>


namespace libbio {
	
	void vcf_info_field_base::prepare(variant_base &dst) const
	{
		reset(dst.m_info.get());
	}
	
	
	void vcf_info_field_base::parse_and_assign(std::string_view const &sv, transient_variant &dst) const
	{
		libbio_assert(m_metadata);
		auto const did_assign(parse_and_assign(sv, dst, dst.m_info.get()));
		dst.m_assigned_info_fields[m_metadata->get_index()] = did_assign;
	}
	
	
	void vcf_info_field_base::assign_flag(transient_variant &dst) const
	{
		auto const vt(value_type());
		if (vcf_metadata_value_type::NOT_PROCESSED == vt)
			return;
		
		if (vcf_metadata_value_type::FLAG != vt)
			throw std::runtime_error("Field type is not FLAG");
		auto const did_assign(assign(dst.m_info.get()));
		
		libbio_assert(m_metadata);
		dst.m_assigned_info_fields[m_metadata->get_index()] = did_assign;
	}
	
	
	void vcf_genotype_field_base::prepare(variant_sample &dst) const
	{
		reset(dst.m_sample_data.get());
	}
	
	
	void vcf_genotype_field_base::parse_and_assign(std::string_view const &sv, variant_sample &dst) const
	{
		parse_and_assign(sv, dst, dst.m_sample_data.get());
	}
	
	
	// Map VCF types to C++ types. Use a byte for FLAG since there are no means for using bit fields right now.
	template <vcf_metadata_value_type t_metadata_value_type> struct vcf_field_type_mapping {};
	template <> struct vcf_field_type_mapping <vcf_metadata_value_type::FLAG>		{ typedef std::uint8_t type; };
	template <> struct vcf_field_type_mapping <vcf_metadata_value_type::INTEGER>	{ typedef std::int32_t type; };
	template <> struct vcf_field_type_mapping <vcf_metadata_value_type::FLOAT>		{ typedef float type; };
	template <> struct vcf_field_type_mapping <vcf_metadata_value_type::STRING>		{ typedef std::string_view type; };
	template <> struct vcf_field_type_mapping <vcf_metadata_value_type::CHARACTER>	{ typedef std::string_view type; };
	
	template <vcf_metadata_value_type t_metadata_value_type>
	using vcf_field_type_mapping_t = typename vcf_field_type_mapping <t_metadata_value_type>::type;
	
	
	// Helper for constructing a vector.
	// Placement new is needed, so std::make_from_tuple cannot be used.
	// Default implementation for various values including VCF_NUMBER_DETERMINED_AT_RUNTIME.
	template <std::int32_t t_number>
	struct vcf_vector_value_helper
	{
		// Construct a vector with placement new. Allocate the given amount of memory.
		template <typename t_vector, typename t_metadata>
		static void construct_ds(std::byte *mem, std::uint16_t const alt_count, t_metadata const &metadata)
		{
			if constexpr (0 < t_number)
				new (mem) t_vector(t_number);
			else
			{
				auto const number(metadata.get_number());
				if (0 < number)
					new (mem) t_vector(number);
				else
					new (mem) t_vector();
			}
		}
	};
	
	// Special cases.
	template <>
	struct vcf_vector_value_helper <VCF_NUMBER_ONE_PER_ALTERNATE_ALLELE>
	{
		template <typename t_vector>
		static void construct_ds(std::byte *mem, std::uint16_t const alt_count, vcf_metadata_base const &)
		{
			new (mem) t_vector(alt_count);
		}
	};
	
	template <>
	struct vcf_vector_value_helper <VCF_NUMBER_ONE_PER_ALLELE>
	{
		template <typename t_vector>
		static void construct_ds(std::byte *mem, std::uint16_t const alt_count, vcf_metadata_base const &)
		{
			new (mem) t_vector(1 + alt_count);
		}
	};
	
	
	// Handle VCF values in the reserved memory.
	template <typename t_type>
	struct vcf_value_access_base
	{
		// Not currently needed b.c. all memory for INFO fields and samples is preallocated.
		//static_assert(std::is_trivially_move_constructible_v <t_type>);
		//static_assert(std::is_trivially_move_assignable_v <t_type>);
		
		// Construct t_type with placement new.
		static void construct_ds(std::byte *mem, std::uint16_t const alt_count, vcf_metadata_base const &)
		{
			libbio_always_assert_eq(0, reinterpret_cast <std::uintptr_t>(mem) % alignof(t_type));
			new (mem) t_type{};
		}
		
		// Call the destructor, no-op for primitive types.
		static void destruct_ds(std::byte *mem) {}
		
		// Access the value, return a reference.
		static constexpr t_type &access_ds(std::byte *mem) { return *reinterpret_cast <t_type *>(mem); }
		static constexpr t_type const &access_ds(std::byte const *mem) { return *reinterpret_cast <t_type const *>(mem); }
		
		// Copy the value.
		static void copy_ds(std::byte const *src, std::byte *dst)
		{
			auto const &srcv(access_ds(src));
			auto &dstv(access_ds(dst));
			dstv = srcv;
		}
		
		// Reset the value for new variant or sample.
		static void reset_ds(std::byte *mem)
		{
			// No-op.
		}
		
		// Determine the byte size.
		static constexpr std::size_t byte_size() { return sizeof(t_type); }
		
		// Determine the alignment.
		static constexpr std::size_t alignment() { return alignof(t_type); }
		
		// Replace or add a value.
		static constexpr void add_value(std::byte *mem, t_type const &val) { access_ds(mem) = val; }
		
		static void output_vcf_value(std::ostream &stream, std::byte *mem) { stream << access_ds(mem); }
	};
	
	
	// Handle primitive values in the reserved memory.
	template <typename t_type>
	struct vcf_primitive_value_access : public vcf_value_access_base <t_type> {};
	
	
	// Special output rules for integers.
	template <>
	struct vcf_primitive_value_access <std::int32_t> : public vcf_value_access_base <std::int32_t>
	{
		static void output_vcf_value(std::ostream &stream, std::byte *mem)
		{
			auto const value(access_ds(mem));
			::libbio::output_vcf_value(stream, value);
		}
	};
	
	
	// Handle object values in the reserved memory.
	template <typename t_type>
	struct vcf_object_value_access : public vcf_value_access_base <t_type>
	{
		// Call t_type’s destructor.
		static void destruct_ds(std::byte *mem)
		{
			reinterpret_cast <t_type *>(mem)->~t_type();
		}
	};
	
	
	// Handle vector values in the reserved memory.
	// Currently, a vector is allocated for all vector types except GT.
	template <typename t_element_type, std::int32_t t_number>
	struct vcf_vector_value_access : public vcf_object_value_access <std::vector <t_element_type>>
	{
		typedef std::vector <t_element_type>	vector_type;
		
		using vcf_object_value_access <std::vector <t_element_type>>::access_ds;
		
		template <typename t_metadata>
		static void construct_ds(std::byte *mem, std::uint16_t const alt_count, t_metadata const &metadata)
		{
			libbio_always_assert_eq(0, reinterpret_cast <std::uintptr_t>(mem) % alignof(vector_type));
			vcf_vector_value_helper <t_number>::template construct_ds <vector_type>(mem, alt_count, metadata);
		}
		
		static void reset_ds(std::byte *mem)
		{
			access_ds(mem).clear();
		}
		
		// Replace or add a value.
		static constexpr void add_value(std::byte *mem, t_element_type const &val) { access_ds(mem).emplace_back(val); }
		
		// Output the vector contents separated by “,”.
		static void output_vcf_value(std::ostream &stream, std::byte *mem)
		{
			auto const &vec(access_ds(mem));
			ranges::copy(vec, ranges::make_ostream_joiner(stream, ","));
		}
	};
	
	
	// Use an alias template to make writing the vector (non-)specialization (below) easier.
	template <std::int32_t t_number, vcf_metadata_value_type t_metadata_value_type>
	using vcf_vector_value_access_t = vcf_vector_value_access <vcf_field_type_mapping_t <t_metadata_value_type>, t_number>;
	
	
	// Handle a VCF field (specified in ##INFO or ##FORMAT).
	// (Non-)specialization for vectors.
	template <std::int32_t t_number, vcf_metadata_value_type t_metadata_value_type>
	struct vcf_subfield_access : public vcf_vector_value_access_t <t_number, t_metadata_value_type>
	{
		typedef vcf_vector_value_access_t <t_number, t_metadata_value_type>	value_access_type;
		typedef typename value_access_type::vector_type						value_type;
		typedef typename value_type::value_type								element_type;
	};
	
	// Specialization for strings.
	template <> struct vcf_subfield_access <1, vcf_metadata_value_type::STRING> final : public vcf_object_value_access <std::string_view>
	{
		typedef std::string_view	value_type;
		typedef value_type			element_type;
	};
	
	// Specializations for scalar values.
	// FIXME: make it possible to store individual bits for flags?
	template <> struct vcf_subfield_access <1, vcf_metadata_value_type::FLOAT> final : public vcf_primitive_value_access <float>
	{
		typedef float				value_type;
		typedef value_type			element_type;
	};
	
	template <> struct vcf_subfield_access <1, vcf_metadata_value_type::INTEGER> final : public vcf_primitive_value_access <std::int32_t>
	{
		typedef std::int32_t		value_type;
		typedef value_type			element_type;
	};
	
	template <> struct vcf_subfield_access <0, vcf_metadata_value_type::FLAG> final : public vcf_primitive_value_access <std::uint8_t>
	{
		typedef std::uint8_t		value_type;
		typedef value_type			element_type;
	};
	
	
	// VCF value parser. The main parser handles vectors, so this class only needs to handle single values.
	// Since the value type is specified in the headers, making the parser stateful enough to parse the values consistently would be
	// more difficult than doing the parsing here.
	// Non-specialization intentionally left blank.
	template <vcf_metadata_value_type t_metadata_value_type>
	struct vcf_subfield_parser {};
	
	struct vcf_subfield_parser_base { static constexpr bool type_needs_parsing() { return true; } };
	
	// Specializations for the different value types.
	template <> struct vcf_subfield_parser <vcf_metadata_value_type::INTEGER> : public vcf_subfield_parser_base
	{
		typedef std::int32_t value_type;
		static void parse(std::string_view const &sv, value_type &dst);
	};
	
	template <> struct vcf_subfield_parser <vcf_metadata_value_type::FLOAT> : public vcf_subfield_parser_base
	{
		typedef float value_type;
		static void parse(std::string_view const &sv, value_type &dst);
	};
	
	template <> struct vcf_subfield_parser <vcf_metadata_value_type::STRING> : public vcf_subfield_parser_base
	{
		static constexpr bool type_needs_parsing() { return false; }
	};
	
	template <> struct vcf_subfield_parser <vcf_metadata_value_type::CHARACTER> : public vcf_subfield_parser_base
	{
		static constexpr bool type_needs_parsing() { return false; }
	};
	
	
	// Helper for implementing parse_and_assign(). Handle vectors here.
	template <std::int32_t t_number, vcf_metadata_value_type t_metadata_value_type>
	class vcf_generic_field_parser
	{
	protected:
		typedef vcf_subfield_access <t_number, t_metadata_value_type>	field_access;
		
	protected:
		void parse_and_assign(std::string_view const &sv, std::byte *mem) const
		{
			// mem needs to include the offset.
			if constexpr (t_metadata_value_type == vcf_metadata_value_type::FLAG)
				libbio_fail("parse_and_assign should not be called for FLAG type fields");
			else
			{
				typedef vcf_subfield_parser <t_metadata_value_type> parser_type;
				if constexpr (parser_type::type_needs_parsing())
				{
					std::size_t start_pos(0);
					while (true)
					{
						typename parser_type::value_type value{};
						auto const end_pos(sv.find_first_of(',', start_pos));
						auto const length(end_pos == std::string_view::npos ? end_pos : end_pos - start_pos);
						auto const sv_part(sv.substr(start_pos, length)); // npos is valid for end_pos.
						
						parser_type::parse(sv_part, value);
						field_access::add_value(mem, value);
						
						if (std::string_view::npos == end_pos)
							break;
						
						start_pos = 1 + end_pos;
					}
				}
				else
				{
					field_access::add_value(mem, sv);
				}
			}
		}
	};
	
	// Non-specialization for values with zero elements.
	template <vcf_metadata_value_type t_metadata_value_type>
	struct vcf_generic_field_parser <0, t_metadata_value_type>
	{
		void parse_and_assign(std::string_view const &sv, std::byte *mem) const
		{
			libbio_fail("parse_and_assign should not be called for FLAG type fields");
		}
	};
	
	// Specialization for scalar values.
	template <vcf_metadata_value_type t_metadata_value_type>
	class vcf_generic_field_parser <1, t_metadata_value_type>
	{
	protected:
		typedef vcf_subfield_access <1, t_metadata_value_type>	field_access;
		
	protected:
		void parse_and_assign(std::string_view const &sv, std::byte *mem) const
		{
			// mem needs to include the offset.
			if constexpr (t_metadata_value_type == vcf_metadata_value_type::FLAG)
				libbio_fail("parse_and_assign should not be called for FLAG type fields");
			else
			{
				typedef vcf_subfield_parser <t_metadata_value_type> parser_type;
				if constexpr (parser_type::type_needs_parsing())
				{
					typename parser_type::value_type value{};
					parser_type::parse(sv, value);
					field_access::add_value(mem, value);
				}
				else
				{
					field_access::add_value(mem, sv);
				}
			}
		}
	};

	// Base classes for the generic field types, implement byte_size().
	template <std::int32_t t_number, vcf_metadata_value_type t_metadata_value_type>
	class vcf_generic_info_field_base :
		public vcf_storable_info_field_base,
		public vcf_generic_field_parser <t_number, t_metadata_value_type>
	{
	public:
		typedef variant_base												container_type;
		
	protected:
		typedef vcf_subfield_access <t_number, t_metadata_value_type>		field_access;
		typedef vcf_generic_field_parser <t_number, t_metadata_value_type>	parser_type;
		
	protected:
		// Access the container’s buffer, for use with operator().
		std::byte *buffer_start(container_type const &ct) const { return ct.m_info.get(); }
		
		// Handle boolean values.
		virtual bool assign(std::byte *mem) const override
		{
			libbio_always_assert_neq(vcf_storable_subfield_base::INVALID_OFFSET, m_offset);
			field_access::add_value(mem + m_offset, 0);
			return true;
		};
		
		virtual bool parse_and_assign(std::string_view const &sv, variant_base &var, std::byte *mem) const override
		{
			libbio_always_assert_neq(vcf_storable_subfield_base::INVALID_OFFSET, m_offset);
			parser_type::parse_and_assign(sv, mem + m_offset);
			return true;
		}
	};
	
	template <std::int32_t t_number, vcf_metadata_value_type t_metadata_value_type>
	class vcf_generic_genotype_field_base :
		public vcf_storable_genotype_field_base,
		public vcf_generic_field_parser <t_number, t_metadata_value_type>
	{
	public:
		typedef variant_sample												container_type;
		
	protected:
		typedef vcf_generic_field_parser <t_number, t_metadata_value_type>	parser_type;
		
	protected:
		// Access the container’s buffer, for use with operator().
		std::byte *buffer_start(container_type const &ct) const { return ct.m_sample_data.get(); }
		
		virtual bool parse_and_assign(std::string_view const &sv, variant_sample &var, std::byte *mem) const override
		{
			libbio_always_assert_neq(vcf_storable_subfield_base::INVALID_OFFSET, m_offset);
			parser_type::parse_and_assign(sv, mem + m_offset);
			return true;
		}
	};
	
	// A generic field type.
	// Delegate the member functions to helper classes.
	template <
		std::int32_t t_number,
		vcf_metadata_value_type t_metadata_value_type,
		template <std::int32_t, vcf_metadata_value_type> class t_base
	>
	class vcf_generic_field final : public t_base <t_number, t_metadata_value_type>
	{
	protected:
		typedef variant_base											container_type;
		typedef vcf_subfield_access <t_number, t_metadata_value_type>	field_access;
		typedef t_base <t_number, t_metadata_value_type>				base_class;
		
	protected:
		virtual std::uint16_t alignment() const override { return field_access::alignment(); }
		virtual std::int32_t number() const override { return t_number; }
		virtual std::uint16_t byte_size() const override { return field_access::byte_size(); }
		
		// Construct the type in the memory block. The additional parameters are passed to the constructor if needed.
		virtual void construct_ds(std::byte *mem, std::uint16_t const alt_count) const override
		{
			libbio_always_assert_neq(vcf_storable_subfield_base::INVALID_OFFSET, this->m_offset);
			field_access::construct_ds(mem + this->m_offset, alt_count, *this->m_metadata);
		}
		
		// Destruct the type in the memory block (if needed).
		virtual void destruct_ds(std::byte *mem) const override
		{
			libbio_always_assert_neq(vcf_storable_subfield_base::INVALID_OFFSET, this->m_offset);
			field_access::destruct_ds(mem + this->m_offset);
		}
		
		// Copy the data structures.
		virtual void copy_ds(std::byte const *src, std::byte *dst) const override
		{
			libbio_always_assert_neq(vcf_storable_subfield_base::INVALID_OFFSET, this->m_offset);
			field_access::copy_ds(src + this->m_offset, dst + this->m_offset);
		}
		
		typename field_access::value_type &access_ds(std::byte *mem) const
		{
			libbio_always_assert_neq(vcf_storable_subfield_base::INVALID_OFFSET, this->m_offset);
			return field_access::access_ds(mem + this->m_offset);
		}
		
		virtual void reset(std::byte *mem) const override
		{
			libbio_always_assert_neq(vcf_storable_subfield_base::INVALID_OFFSET, this->m_offset);
			field_access::reset_ds(mem + this->m_offset);
		}
		
		virtual vcf_generic_field *clone() const override { return new vcf_generic_field(*this); }
		
	public:
		virtual vcf_metadata_value_type value_type() const override { return t_metadata_value_type; }

		virtual void output_vcf_value(std::ostream &stream, typename base_class::container_type const &ct) const override
		{
			libbio_always_assert_neq(vcf_storable_subfield_base::INVALID_OFFSET, this->m_offset);
			field_access::output_vcf_value(stream, this->buffer_start(ct) + this->m_offset);
		}
		
		// Convenience function.
		typename field_access::value_type &operator()(typename base_class::container_type const &ct)
		{
			return access_ds(this->buffer_start(ct));
		}
	};
	
	
	template <std::int32_t t_number, vcf_metadata_value_type t_metadata_value_type>
	using vcf_info_field = vcf_generic_field <t_number, t_metadata_value_type, vcf_generic_info_field_base>;

	template <std::int32_t t_number, vcf_metadata_value_type t_metadata_value_type>
	using vcf_genotype_field = vcf_generic_field <t_number, t_metadata_value_type, vcf_generic_genotype_field_base>;
	
	
	// Info fields.
	using vcf_info_field_aa			= vcf_info_field <1,										vcf_metadata_value_type::STRING>;
	using vcf_info_field_ac			= vcf_info_field <VCF_NUMBER_ONE_PER_ALTERNATE_ALLELE,		vcf_metadata_value_type::INTEGER>;
	using vcf_info_field_ad			= vcf_info_field <VCF_NUMBER_ONE_PER_ALLELE,				vcf_metadata_value_type::INTEGER>;
	using vcf_info_field_adf		= vcf_info_field <VCF_NUMBER_ONE_PER_ALLELE,				vcf_metadata_value_type::INTEGER>;
	using vcf_info_field_adr		= vcf_info_field <VCF_NUMBER_ONE_PER_ALLELE,				vcf_metadata_value_type::INTEGER>;
	using vcf_info_field_af			= vcf_info_field <VCF_NUMBER_ONE_PER_ALTERNATE_ALLELE,		vcf_metadata_value_type::FLOAT>;
	using vcf_info_field_an			= vcf_info_field <1,										vcf_metadata_value_type::INTEGER>;
	using vcf_info_field_bq			= vcf_info_field <1,										vcf_metadata_value_type::FLOAT>;
	using vcf_info_field_cigar		= vcf_info_field <VCF_NUMBER_ONE_PER_ALTERNATE_ALLELE,		vcf_metadata_value_type::STRING>;
	using vcf_info_field_db			= vcf_info_field <0,										vcf_metadata_value_type::FLAG>;
	using vcf_info_field_dp			= vcf_info_field <1,										vcf_metadata_value_type::INTEGER>;
	using vcf_info_field_end		= vcf_info_field <1,										vcf_metadata_value_type::INTEGER>;
	using vcf_info_field_h2			= vcf_info_field <0,										vcf_metadata_value_type::FLAG>;
	using vcf_info_field_h3			= vcf_info_field <0,										vcf_metadata_value_type::FLAG>;
	using vcf_info_field_mq			= vcf_info_field <1,										vcf_metadata_value_type::FLOAT>;
	using vcf_info_field_mq0		= vcf_info_field <1,										vcf_metadata_value_type::INTEGER>;
	using vcf_info_field_ns			= vcf_info_field <1,										vcf_metadata_value_type::INTEGER>;
	using vcf_info_field_sb			= vcf_info_field <4,										vcf_metadata_value_type::INTEGER>;
	using vcf_info_field_somatic	= vcf_info_field <0,										vcf_metadata_value_type::FLAG>;
	using vcf_info_field_validated	= vcf_info_field <0,										vcf_metadata_value_type::FLAG>;
	using vcf_info_field_1000g		= vcf_info_field <0,										vcf_metadata_value_type::FLAG>;
	
	// Genotype fields.
	using vcf_genotype_field_ad		= vcf_genotype_field <VCF_NUMBER_ONE_PER_ALLELE,			vcf_metadata_value_type::INTEGER>;
	using vcf_genotype_field_adf	= vcf_genotype_field <VCF_NUMBER_ONE_PER_ALLELE,			vcf_metadata_value_type::INTEGER>;
	using vcf_genotype_field_adr	= vcf_genotype_field <VCF_NUMBER_ONE_PER_ALLELE,			vcf_metadata_value_type::INTEGER>;
	using vcf_genotype_field_dp		= vcf_genotype_field <1,									vcf_metadata_value_type::INTEGER>;
	using vcf_genotype_field_ec		= vcf_genotype_field <VCF_NUMBER_ONE_PER_ALTERNATE_ALLELE,	vcf_metadata_value_type::INTEGER>;
	using vcf_genotype_field_ft		= vcf_genotype_field <1,									vcf_metadata_value_type::STRING>;
	using vcf_genotype_field_gl		= vcf_genotype_field <VCF_NUMBER_ONE_PER_GENOTYPE,			vcf_metadata_value_type::FLOAT>;
	using vcf_genotype_field_gp		= vcf_genotype_field <VCF_NUMBER_ONE_PER_GENOTYPE,			vcf_metadata_value_type::FLOAT>;
	using vcf_genotype_field_gq		= vcf_genotype_field <1,									vcf_metadata_value_type::INTEGER>;
	using vcf_genotype_field_hq		= vcf_genotype_field <2,									vcf_metadata_value_type::INTEGER>;
	using vcf_genotype_field_mq		= vcf_genotype_field <1,									vcf_metadata_value_type::INTEGER>;
	using vcf_genotype_field_pl		= vcf_genotype_field <VCF_NUMBER_ONE_PER_GENOTYPE,			vcf_metadata_value_type::INTEGER>;
	using vcf_genotype_field_pq		= vcf_genotype_field <1,									vcf_metadata_value_type::INTEGER>;
	using vcf_genotype_field_ps		= vcf_genotype_field <1,									vcf_metadata_value_type::INTEGER>;
	
	
	// Specialization for the GT field.
	class vcf_genotype_field_gt final : public vcf_genotype_field_base
	{
	protected:
		typedef std::vector <sample_genotype> vector_type;
			
	protected:
		virtual std::uint16_t alignment() const override { return 1; }
		virtual std::uint16_t byte_size() const override { return 0; }
		
		virtual std::int32_t number() const override { return 1; }
		
		virtual void construct_ds(std::byte *mem, std::uint16_t const alt_count) const override
		{
			// No-op.
		}
		
		virtual void destruct_ds(std::byte *mem) const override
		{
			// No-op.
		}
		
		virtual void copy_ds(std::byte const *src, std::byte *dst) const override
		{
			// No-op.
		}
		
		virtual void reset(std::byte *mem) const override
		{
			// No-op, done in parse_and_assign.
		}
		
		virtual bool parse_and_assign(std::string_view const &sv, variant_sample &sample, std::byte *mem) const override;
		
		virtual vcf_genotype_field_gt *clone() const override { return new vcf_genotype_field_gt(*this); }
		
	public:
		virtual vcf_metadata_value_type value_type() const override { return vcf_metadata_value_type::STRING; }
		virtual void output_vcf_value(std::ostream &stream, variant_sample const &var) const override;

		// Convenience function.
		vector_type const &operator()(variant_sample const &ct) const
		{
			return ct.m_genotype;
		}
	};
}

#endif

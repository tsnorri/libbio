/*
 * Copyright (c) 2020 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_SUBFIELD_VALUE_ACCESS_HH
#define LIBBIO_VCF_SUBFIELD_VALUE_ACCESS_HH

#include <libbio/assert.hh>
#include <libbio/types.hh>
#include <libbio/vcf/subfield/type_mapping.hh>
#include <libbio/vcf/subfield/vector_value_helper.hh>
#include <libbio/vcf/metadata.hh>
#include <ostream>
#include <range/v3/algorithm/copy.hpp>
#include <range/v3/iterator/stream_iterators.hpp>
#include <vector>


namespace libbio { namespace detail {
	
	template <typename t_src, typename t_dst>
	void copy_vector(std::vector <t_src> const &src, std::vector <t_dst> &dst)
	{
		auto const size(src.size());
		dst.resize(size);
		for (std::size_t i(0); i < size; ++i)
			dst[i] = src[i];
	}
}}


namespace libbio {
	
	// Handle VCF values in the reserved memory.
	template <typename t_type>
	struct vcf_value_access_base
	{
		// Not currently needed b.c. all memory for INFO fields and samples is preallocated.
		//static_assert(std::is_trivially_move_constructible_v <t_type>);
		//static_assert(std::is_trivially_move_assignable_v <t_type>);
		
		// Access only by using static member functions.
		vcf_value_access_base() = delete;
		
		// Construct t_type with placement new.
		static void construct_ds(std::byte *mem, std::uint16_t const alt_count, vcf_metadata_base const &)
		{
			libbio_always_assert_eq(0, reinterpret_cast <std::uintptr_t>(mem) % alignof(t_type));
			new (mem) t_type{};
		}
		
		// Call the destructor, no-op for primitive types.
		static void destruct_ds(std::byte *mem) {}
		
		// Access the value, return a reference.
		static t_type &access_ds(std::byte *mem) { return *reinterpret_cast <t_type *>(mem); }
		static t_type const &access_ds(std::byte const *mem) { return *reinterpret_cast <t_type const *>(mem); }
		
		// Copy the value. Should only be called for copying the data to a non-transient variant.
		static void copy_ds(std::byte const *src, std::byte *dst)
		{
			auto const &srcv(access_ds(src));
			auto &dstv(access_ds(dst));
			dstv = srcv;
		}
		
		// Reset the value for new variant or sample.
		static void reset_ds(std::byte *mem) { /* No-op. */ }
		
		// Determine the byte size.
		static constexpr std::size_t byte_size() { return sizeof(t_type); }
		
		// Determine the alignment.
		static constexpr std::size_t alignment() { return alignof(t_type); }
		
		// Replace or add a value.
		// This works for string views b.c. the buffer where the characters are located still exists.
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
	struct vcf_vector_value_access_base : public vcf_object_value_access <std::vector <t_element_type>>
	{
		static_assert(vcf_value_count_corresponds_to_vector(t_number));
		typedef std::vector <t_element_type>	vector_type;
		
		using vcf_object_value_access <vector_type>::access_ds;
		
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
		static void add_value(std::byte *mem, t_element_type const &val) { access_ds(mem).emplace_back(val); }
		
		// Output the vector contents separated by “,”.
		static void output_vcf_value(std::ostream &stream, std::byte *mem)
		{
			auto const &vec(access_ds(mem));
			ranges::copy(vec, ranges::make_ostream_joiner(stream, ","));
		}
	};
	
	
	// Non-specialization for everything except std::string_view.
	template <typename t_element_type, std::int32_t t_number>
	struct vcf_vector_value_access : public vcf_vector_value_access_base <t_element_type, t_number>
	{
	};
	
	// Specialization to handle copying std::vector <std::string_view> to std::vector <std::string>.
	template <std::int32_t t_number>
	struct vcf_vector_value_access <std::string_view, t_number> :
		public vcf_vector_value_access_base <std::string_view, t_number>
	{
		using vcf_vector_value_access_base <std::string_view, t_number>::access_ds;
		
		// Copy the value. Should only be called for copying the data to a non-transient variant.
		static void copy_ds(std::byte const *src, std::byte *dst)
		{
			auto const &srcv(access_ds(src));
			auto &dstv(vcf_vector_value_access <std::string, t_number>::access_ds(dst));
			detail::copy_vector(srcv, dstv);
		}
	};
}

#endif

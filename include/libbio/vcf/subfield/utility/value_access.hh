/*
 * Copyright (c) 2020 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_SUBFIELD_VALUE_ACCESS_HH
#define LIBBIO_VCF_SUBFIELD_VALUE_ACCESS_HH

#include <libbio/assert.hh>
#include <libbio/types.hh>
#include <libbio/vcf/subfield/utility/type_mapping.hh>
#include <libbio/vcf/subfield/utility/vector_value_helper.hh>
#include <libbio/vcf/metadata.hh>
#include <ostream>
#include <range/v3/algorithm/copy.hpp>
#include <range/v3/iterator/stream_iterators.hpp>
#include <vector>


namespace libbio::vcf {
	
	// Handle VCF values in the reserved memory.
	template <typename t_type>
	struct value_access_base
	{
		typedef t_type value_type;
		
		// Not currently needed b.c. all memory for INFO fields and samples is preallocated.
		//static_assert(std::is_trivially_move_constructible_v <value_type>);
		//static_assert(std::is_trivially_move_assignable_v <value_type>);
		
		// Access only by using static member functions.
		value_access_base() = delete;
		
		// Construct value_type with placement new.
		static void construct_ds(std::byte *mem, std::uint16_t const alt_count, metadata_base const &)
		{
			libbio_always_assert_eq(0, reinterpret_cast <std::uintptr_t>(mem) % alignof(value_type));
			new (mem) value_type{};
		}
		
		// Call the destructor, no-op for primitive types.
		constexpr static void destruct_ds(std::byte *mem) {}
		
		// Access the value, return a reference.
		constexpr static value_type &access_ds(std::byte *mem)
		{
			auto &val(*reinterpret_cast <value_type *>(mem));
			return val;
		}
		
		constexpr static value_type const &access_ds(std::byte const *mem)
		{
			auto const &val(*reinterpret_cast <value_type const *>(mem));
			return val;
		}
		
		// Reset the value for new variant or sample.
		constexpr static void reset_ds(std::byte *mem) { /* No-op. */ }
		
		// Determine the byte size.
		constexpr static std::size_t byte_size() { return sizeof(value_type); }
		
		// Determine the alignment.
		constexpr static std::size_t alignment() { return alignof(value_type); }
		
		// Replace or add a value.
		// This works for string views b.c. the buffer where the characters are located still exists.
		constexpr static void add_value(std::byte *mem, value_type const &val)
		{
			auto &dst(access_ds(mem));
			dst = val;
		}
		
		static void output_vcf_value(std::ostream &stream, std::byte *mem) { stream << access_ds(mem); }
	};
	
	
	// Handle primitive values in the reserved memory.
	template <typename t_type>
	struct primitive_value_access : public value_access_base <t_type> {};
	
	// Special output rules for integers.
	template <>
	struct primitive_value_access <std::int32_t> : public value_access_base <std::int32_t>
	{
		typedef std::int32_t value_type;
		
		static void output_vcf_value(std::ostream &stream, std::byte *mem)
		{
			auto const value(access_ds(mem));
			::libbio::vcf::output_vcf_value(stream, value);
		}
	};
	
	
	// Handle object values in the reserved memory.
	template <typename t_type>
	struct object_value_access : public value_access_base <t_type>
	{
		typedef t_type value_type;
		
		// Call value_type’s destructor.
		static void destruct_ds(std::byte *mem)
		{
			reinterpret_cast <value_type *>(mem)->~value_type();
		}
	};
	
	
	// Handle vector values in the reserved memory.
	template <typename t_element_type, std::int32_t t_number>
	struct vector_value_access : public object_value_access <std::vector <t_element_type>>
	{
		static_assert(value_count_corresponds_to_vector(t_number));
		typedef std::vector <t_element_type>	vector_type;
		typedef vector_type						value_type;
		
		using object_value_access <vector_type>::access_ds;
		
		template <typename t_metadata>
		static void construct_ds(std::byte *mem, std::uint16_t const alt_count, t_metadata const &metadata)
		{
			libbio_always_assert_eq(0, reinterpret_cast <std::uintptr_t>(mem) % alignof(vector_type));
			vector_value_helper <t_number>::template construct_ds <vector_type>(mem, alt_count, metadata);
		}
		
		static void reset_ds(std::byte *mem)
		{
			access_ds(mem).clear();
		}
		
		// Replace or add a value.
		static void add_value(std::byte *mem, t_element_type const &val)
		{
			auto &dst(access_ds(mem));
			dst.emplace_back(val);
		}
		
		// Output the vector contents separated by “,”.
		static void output_vcf_value(std::ostream &stream, std::byte *mem)
		{
			auto const &vec(access_ds(mem));
			ranges::copy(vec, ranges::make_ostream_joiner(stream, ","));
		}
	};
}

#endif

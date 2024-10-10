/*
 * Copyright (c) 2020-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_SUBFIELD_VECTOR_VALUE_HELPER_HH
#define LIBBIO_VCF_SUBFIELD_VECTOR_VALUE_HELPER_HH

#include <libbio/vcf/constants.hh>
#include <libbio/vcf/metadata.hh>


namespace libbio::vcf {
	
	// Helper for constructing a vector.
	// Placement new is needed, so std::make_from_tuple cannot be used.
	// Default implementation for various values including VCF_NUMBER_DETERMINED_AT_RUNTIME.
	template <std::int32_t t_number>
	struct vector_value_helper
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
	struct vector_value_helper <VCF_NUMBER_ONE_PER_ALTERNATE_ALLELE>
	{
		template <typename t_vector>
		static void construct_ds(std::byte *mem, std::uint16_t const alt_count, metadata_base const &)
		{
			new (mem) t_vector(alt_count);
		}
	};
	
	template <>
	struct vector_value_helper <VCF_NUMBER_ONE_PER_ALLELE>
	{
		template <typename t_vector>
		static void construct_ds(std::byte *mem, std::uint16_t const alt_count, metadata_base const &)
		{
			new (mem) t_vector(1 + alt_count);
		}
	};
}

#endif

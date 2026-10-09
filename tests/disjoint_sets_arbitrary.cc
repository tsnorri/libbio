/*
 * Copyright (c) 2026 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <libbio/disjoint_sets.hh>
#include <libbio/rapidcheck_test_driver.hh>
#include <limits>
#include <numeric>
#include <random>
#include <vector>

namespace lb	= libbio;


namespace {

	typedef std::uint8_t partition_count_type;
	typedef std::vector <partition_count_type> partition_vector;
	typedef std::size_t index_type;
	typedef std::vector <index_type> index_vector;

	constexpr static inline partition_count_type PARTITION_COUNT_MAX{std::numeric_limits <partition_count_type>::max()};


	struct test_input
	{
		index_vector indices{};
		partition_vector partitions{};
		partition_count_type partition_count{};
	};
}


namespace rc {

	template <>
	struct Arbitrary <test_input>
	{
		static Gen <test_input> arbitrary()
		{
			return gen::withSize([](int size){
				RC_ASSERT(0 <= size);
				if (0 == size)
					return gen::just(test_input{});

				partition_count_type const partition_count((size + 4) / 3);
				RC_ASSERT(0 < partition_count);

				return gen::mapcat(gen::arbitrary <partition_count_type>(), [size, partition_count](partition_count_type const seed){
					std::default_random_engine rng{seed};

					index_vector indices(size);
					std::iota(indices.begin(), indices.end(), 0);
					std::shuffle(indices.begin(), indices.end(), rng);

					return gen::construct <test_input>(
						gen::just(std::move(indices)),
						gen::container <partition_vector>(size, gen::inRange(partition_count_type{}, partition_count)),
						gen::just(partition_count)
					);
				});
			});
		}
	};
}


TEST_CASE(
	"disjoint_sets’ operations work as expected",
	"[disjoint_sets]"
)
{
	return lb::rc_check(
		"disjoint_sets’s operations work as expected",
		[](test_input const &input){
			RC_ASSERT(input.partitions.size() == input.indices.size());

			auto const size{input.partitions.size()};
			lb::disjoint_sets <index_vector> ds{size};

			if (0 == size) return;

			// Check that every set is initially a singleton.
			for (index_type ii{}; ii < size; ++ii)
			{
				RC_ASSERT(ds.find(ii) == ii);
				RC_ASSERT(ds.is_singleton(ii));
			}

			// Iterate in the given order, call set_union where needed.
			index_vector representatives(input.partition_count - 1, PARTITION_COUNT_MAX);
			for (auto const idx : input.indices)
			{
				auto const partition{input.partitions[idx]};
				if (0 == partition)
				{
					// Singleton.
					continue;
				}

				auto const representative{representatives[partition - 1]};
				if (PARTITION_COUNT_MAX == representative)
				{
					representatives[partition - 1] = idx;
					continue;
				}

				ds.set_union(representative, idx);
			}

			// Check.
			for (index_type ii{}; ii < size; ++ii)
			{
				auto const partition{input.partitions[ii]};
				if (0 == partition)
				{
					RC_ASSERT(ds.find(ii) == ii);
					RC_ASSERT(ds.is_singleton(ii));
					continue;
				}

				auto const expected{representatives[partition - 1]};
				auto const actual{ds.find(ii)};
				RC_ASSERT(expected == actual);
			}
		}
	);
}

/*
 * Copyright (c) 2022-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <algorithm>
#include <cstddef>
#include <libbio/vcf/region_variant_validator.hh>
#include <libbio/vcf/variant.hh>
#include <libbio/vcf/vcf_reader_decl.hh>
#include <string>
#include <string_view>


namespace libbio::vcf {

	variant_validation_result region_variant_validator::validate(transient_variant const &var)
	{
		auto const chr_id(var.chrom_id());
		auto const var_pos(var.zero_based_pos());
		if (m_prev_chr_id == chr_id)
		{
			if (var_pos < m_prev_var_pos)
				return handle_unordered_variants(var);

			m_prev_var_pos = var_pos;
			if (m_should_check_positions)
			{
				if (!m_is_known_region)
				{
					++m_chr_id_mismatches;
					return variant_validation_result::SKIP;
				}

				while (true)
				{
					if (m_range_it == m_range_end)
					{
						++m_position_mismatches;
						return variant_validation_result::SKIP;
					}

					if (m_range_it->end <= var_pos)
					{
						++m_range_it;
						continue;
					}

					// var_pos < m_range_it->end
					if (var_pos < m_range_it->begin)
					{
						++m_position_mismatches;
						return variant_validation_result::SKIP;
					}

					// m_range_it->begin <= var_pos
					return variant_validation_result::PASS;
				}
			}
			else
			{
				return variant_validation_result::PASS;
			}
		}
		else
		{
			m_prev_chr_id = chr_id;
			m_prev_var_pos = var_pos;

			auto const it(m_regions.find(chr_id));
			if (m_regions.end() == it)
			{
				// Add an entry for the current chromosome ID.
				auto const it(m_regions.try_emplace(std::string(chr_id)).first);
				it->second.is_seen = true;

				m_range_it = m_range_end;
				m_is_known_region = false;

				if (m_should_check_positions)
				{
					++m_chr_id_mismatches;
					return variant_validation_result::SKIP;
				}

				return variant_validation_result::PASS;
			}

			// Found an entry for the chromosome ID.
			auto &region(it->second);
			if (region.is_seen)
				return handle_unordered_contigs(var);
			region.is_seen = true;

			m_range_it = region.ranges.begin();
			m_range_end = region.ranges.end();
			m_is_known_region = true;
			return variant_validation_result::PASS;
		}
	}


	// Reports a half-open interval.
	void region_variant_validator_bed_reader_delegate::bed_reader_found_region(
		std::string_view const chr_id,
		std::size_t const begin,
		std::size_t const end
	)
	{
		// try_emplace() does not have an overload that does not construct a key, so we find and emplace instead.
		auto it(m_regions_by_chr_id->find(chr_id));
		if (m_regions_by_chr_id->end() == it)
			it = m_regions_by_chr_id->try_emplace(std::string(chr_id)).first; // try_emplace() takes a parameter pack after the first parameter.

		it->second.ranges.emplace_back(begin, end);
	}


	void region_variant_validator_bed_reader_delegate::bed_reader_did_finish()
	{
		// Sort and remove overlaps.
		position_range_cmp cmp;
		for (auto &kv : *m_regions_by_chr_id)
		{
			auto &vec(kv.second.ranges);
			if (vec.empty())
				continue;

			std::sort(vec.begin(), vec.end(), cmp);

			auto curr(vec.begin());
			auto it(curr + 1);
			while (it != vec.end()) // erase() invalidates the end iterator.
			{
				if (it->begin < curr->end)
				{
					curr->end = std::max(it->end, curr->end);
					it = vec.erase(it);
				}
				else
				{
					curr = it;
					++it;
				}
			}
		}
	}
}

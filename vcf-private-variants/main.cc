/*
 * Copyright (c) 2021 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <libbio/vcf/vcf_reader.hh>
#include <range/v3/view/generate_n.hpp>
#include <set>
#include <vector>
#include "cmdline.h"

namespace lb	= libbio;
namespace rsv	= ranges::view;
namespace vcf	= libbio::vcf;


namespace {
	
	// From libvcf2multialign.
	// FIXME: come up with a way not to duplicate the code needed for storing field pointers.
	struct variant_format final : public vcf::variant_format
	{
		vcf::genotype_field_gt	*gt_field{};
		
		// Return a new empty instance of this class.
		virtual variant_format *new_instance() const override { return new variant_format(); }
		
		virtual void reader_did_update_format(vcf::reader &reader) override
		{
			this->assign_field_ptr("GT", gt_field);
		}
	};
	
	inline variant_format const &get_variant_format(vcf::variant const &var)
	{
		libbio_assert(var.reader()->has_assigned_variant_format());
		return static_cast <variant_format const &>(var.get_format());
	}
	
	inline variant_format const &get_variant_format(vcf::transient_variant const &var)
	{
		libbio_assert(var.reader()->has_assigned_variant_format());
		return static_cast <variant_format const &>(var.get_format());
	}
}


int main(int argc, char **argv)
{
	gengetopt_args_info args_info;
	if (0 != cmdline_parser(argc, argv, &args_info))
		std::exit(EXIT_FAILURE);
	
	std::ios_base::sync_with_stdio(false);	// Don't use C style IO after calling cmdline_parser.
	
	// Open the variant file.
	vcf::mmap_input vcf_input;
	vcf_input.handle().open(args_info.variants_arg);
	
	vcf::reader reader(vcf_input);
	
	vcf::add_reserved_info_keys(reader.info_fields());
	vcf::add_reserved_genotype_keys(reader.genotype_fields());
		
	// Read the headers.
	reader.set_variant_format(new variant_format());
	reader.read_header();
	
	// Determine the indices of the samples in the given group.
	auto const &sample_indices_by_name(reader.sample_indices_by_name());
	std::vector <std::size_t> private_set(args_info.sample_given, 0);
	for (std::size_t i(0); i < args_info.sample_given; ++i)
	{
		auto const *sample_name(args_info.sample_arg[i]);
		auto const it(sample_indices_by_name.find(sample_name));
		if (sample_indices_by_name.end() == it)
		{
			std::cerr << "ERROR: sample “"  << sample_name << "” not found in VCF.\n";
			std::exit(EXIT_FAILURE);
		}
		
		auto const idx(it->second);
		private_set[i] = idx;
	}
	std::sort(private_set.begin(), private_set.end());
	
	// Should be checked by gengetopt, hence no libbio_always_assert..
	libbio_assert(!private_set.empty());
	
	// Parse the variants.
	std::set <std::uint16_t> private_alts;
	std::set <std::uint16_t> found_alts;
	std::size_t total_count(0);
	reader.set_parsed_fields(vcf::field::ALL);
	reader.parse([&private_set, &private_alts, &found_alts, &total_count](vcf::transient_variant const &var){
		auto const &gt_field(*get_variant_format(var).gt_field);
		auto const &samples(var.samples());
		
		// Set up.
		private_alts.clear();
		found_alts.clear();
		
		// Determine the private ALT values.
		for (auto const sample_idx : private_set)
		{
			libbio_assert_lt(sample_idx, samples.size());
			auto const &sample(samples[sample_idx]);
			auto const &gt(gt_field(sample)); // vector of sample_genotype
			for (auto const &sample_gt : gt)
			{
				// Treat null allele as zero.
				auto const alt(sample_gt.alt);
				private_alts.insert(vcf::sample_genotype::NULL_ALLELE == alt ? 0 : alt);
			}
		}
		
		// If an ALT besides 0 is present, we don’t need to check for zero
		// when handling the complement of private_set.
		if (1 < private_alts.size())
			private_alts.erase(0);
		
		// Determine the indices not in the private set.
		auto const sample_count(samples.size());
		auto rng(rsv::generate_n(
			[it = private_set.begin(), curr_idx = std::size_t(0)]() mutable {
				while (*it <= curr_idx)
				{
					++it;
					++curr_idx;
				}
				
				auto const retval(curr_idx);
				++curr_idx;
				return retval;
			},
			sample_count - private_set.size()
		));
		
		// Handle the complement of private_set.
		for (auto const sample_idx : rng)
		{
			libbio_assert_lt(sample_idx, samples.size());
			auto const &sample(samples[sample_idx]);
			auto const &gt(gt_field(sample)); // vector of sample_genotype
			for (auto const &sample_gt : gt)
			{
				auto const alt(sample_gt.alt);
				auto checked_alt(vcf::sample_genotype::NULL_ALLELE == alt ? 0 : alt);
				if (private_alts.end() != private_alts.find(checked_alt))
				{
					found_alts.insert(checked_alt);
					if (found_alts.size() == private_alts.size())
						return true;
				}
			}
		}
		
		// Not found; we can report the ALTs.
		auto const current_count(private_alts.size() - found_alts.size());
		total_count += current_count;
		std::cout << var.lineno() << '\t' << current_count << '\n';
		return true;
	});
	
	std::cerr << "Found " << total_count << " private variants in total.\n";
	return 0;
}

/*
 * Copyright (c) 2020 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <libbio/vcf/metadata.hh>
#include <libbio/vcf/subfield.hh>
#include <libbio/vcf/variant_format.hh>
#include <libbio/vcf/vcf_reader.hh>
#include <list>
#include <range/v3/all.hpp>
#include "cmdline.h"
#include "variant_printer.hh"


namespace lb	= libbio;
namespace merge	= libbio::vcf_merge_records;
namespace rsv	= ranges::view;
namespace vcf	= libbio::vcf;


namespace {
	
	typedef std::list <vcf::variant>				variant_list;
	typedef std::pair <std::string, std::string>	string_pair;
	
	
	struct indexed_variant
	{
		vcf::variant const	*variant{};
		std::uint16_t		alt_idx{};
		
		indexed_variant(vcf::variant const &variant_, std::uint16_t const alt_idx_):
			variant(&variant_),
			alt_idx(alt_idx_)
		{
		}
	};
	
	
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
	
	
	void output_header(vcf::reader const &reader)
	{
		auto const &metadata(reader.metadata());
		
		std::cout << "##fileformat=VCFv4.3\n";
		metadata.visit_all_metadata([](vcf::metadata_base const &meta){
			meta.output_vcf(std::cout);
		});
		
		std::cout << "#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\tFORMAT";
		for (auto const &name : reader.sample_names_by_index())
			std::cout << '\t' << name;
		std::cout << '\n';
	}
	
	
	void check_phasing(std::vector <indexed_variant> const &variant_group)
	{
		if (variant_group.empty())
			return;
		
		auto const &first_indexed_var(variant_group.front());
		auto const &first_var(*first_indexed_var.variant);
		auto const &first_gt_field(*static_cast <variant_format const &>(first_var.get_format()).gt_field);
		auto const sample_count(first_var.samples().size());
		for (std::size_t i(0); i < sample_count; ++i)
		{
			auto const &first_var_sample(first_var.samples()[i]);
			auto const &first_gt(first_gt_field(first_var_sample));
			auto const ploidy(first_gt.size());
			bool expected_phasing[ploidy];
			for (std::size_t j(0); j < ploidy; ++j)
				expected_phasing[j] = first_gt[j].is_phased;
			
			for (auto const &indexed_var : variant_group | rsv::tail)
			{
				auto const &var(*indexed_var.variant);
				auto const &current_samples(var.samples());
				libbio_always_assert_eq_msg(sample_count, current_samples.size(), "Expected sample counts to match over all variants.");
				auto const &current_sample(current_samples[i]);
				auto const &current_gt_field(*static_cast <variant_format const &>(var.get_format()).gt_field);
				auto const &current_gt(current_gt_field(current_sample));
				libbio_assert_eq_msg(ploidy, current_gt.size(), "Expected ploidy to match in one sample over a group of variants.");
				for (std::size_t j(0); j < ploidy; ++j)
					libbio_assert_eq_msg(expected_phasing[j], current_gt[j].is_phased, "Expected phasing to match in one sample over a group of variants.");
			}
		}
	}
	
	
	void handle_variants(variant_list const &variants_with_same_pos, merge::variant_printer &printer)
	{
		// Sort by REF and ALT.
		std::map <string_pair, std::vector <indexed_variant>> variants_by_ref_and_alt;
		for (auto const &var : variants_with_same_pos)
		{
			auto const &ref(var.ref());
			for (auto const &[idx, alt] : rsv::enumerate(var.alts()))
			{
				string_pair const key(ref, alt.alt);
				variants_by_ref_and_alt[key].emplace_back(var, 1 + idx);
			}
		}
		
		// Output.
		std::vector <std::uint16_t> genotypes;
		std::vector <bool> phasing;
		for (auto const &kv : variants_by_ref_and_alt)
		{
			auto const &key(kv.first);
			auto const &current_variants(kv.second);
			
			auto const &first_var(*current_variants.front().variant);
			auto const &first_gt_field(*static_cast <variant_format const &>(first_var.get_format()).gt_field);
			
			// TODO: allow different ploidies?
			auto const &first_samples(first_var.samples());
			libbio_always_assert_lt_msg(0, first_samples.size(), "Empty samples list handling not implemented.");
			auto const ploidy(first_gt_field(first_samples.front()).size());
			
			genotypes.resize(first_samples.size() * ploidy);
			std::fill(genotypes.begin(), genotypes.end(), 0);
			
			phasing.resize(first_samples.size() * ploidy);
			std::fill(phasing.begin(), phasing.end(), 0);
			
			// Check that the remaining variants match the first one.
			// Currently only GT values are handled.
			for (auto const &indexed_var : rsv::tail(current_variants))
			{
				// TODO: Merge.
				auto const &var(*indexed_var.variant);
				libbio_always_assert_eq(first_samples.size(), var.samples().size());
			}
			
			// Check that the phasing is something that can be handled.
			check_phasing(current_variants);
			
			// Merge the GT values.
			bool is_first_variant(true);
			for (auto const &indexed_var : current_variants)
			{
				auto const &var(*indexed_var.variant);
				auto const current_alt_idx(indexed_var.alt_idx);
				auto const &gt_field(*static_cast <variant_format const &>(var.get_format()).gt_field);
				for (auto const &[i, sample] : rsv::enumerate(var.samples()))
				{
					auto const first_sample_idx(i * ploidy);
					auto const &gt(gt_field(sample));
					
					// List the unphased alleles.
					std::uint16_t unphased_indices[ploidy];
					std::uint16_t unphased_count{};
					bool gt_buffer[ploidy];
					for (auto const &[j, gt_val] : rsv::enumerate(gt))
					{
						{
							auto const idx(i * ploidy + j);
							libbio_assert_lt(idx, phasing.size());
							if (is_first_variant)
								phasing[idx] = gt_val.is_phased;
							else
								libbio_always_assert_eq_msg(phasing[idx], gt_val.is_phased, "Expected phasing to match that of the first sample in the group of variants.");
						}
						
						if (gt_val.is_phased)
							gt_buffer[j] = (current_alt_idx == gt_val.alt);
						else
						{
							// The i-th index in unphased.
							unphased_indices[unphased_count] = j;
							++unphased_count;
						}
					}
					
					// Sort the unphased alleles s.t. 1/0 and 0/1 will be equivalent.
					// TODO: handle phasing over multiple records.
					// (There seems to be no easy solution to this. Suppose a different allele
					// is phased in different variants, e.g. 0|1/1/0 and 0/1/0|1.
					// One possibility would be to consider the bitwise OR of the phased allele
					// indices and output their values as specified in the variants in question,
					// i.e. 0|1/0|1. I’m not sure, though, if this is correct, and how common
					// it is to have partially phased samples.)
					{
						std::uint16_t lhs(0);
						std::uint16_t rhs(unphased_count - 1);
						for (std::uint16_t j(0); j < unphased_count; ++j)
						{
							auto const gt_idx(unphased_indices[j]);
							auto const &gt_val(gt[gt_idx]);
							if (current_alt_idx == gt_val.alt)
							{
								gt_buffer[unphased_indices[rhs]] = 1;
								--rhs;
							}
							else
							{
								gt_buffer[unphased_indices[lhs]] = 0;
								++lhs;
							}
						}
					}
					
					// Store the modified genotypes.
					// TODO: handle phasing in the printer, too.
					for (std::uint16_t j(0); j < ploidy; ++j)
					{
						auto const idx(first_sample_idx + j);
						libbio_assert_lt(idx, genotypes.size());
						genotypes[idx] |= gt_buffer[j];
					}
				}
				
				is_first_variant = false;
			}
			
			printer.set_ref(key.first);
			printer.set_alt(key.second);
			printer.set_ploidy(ploidy);
			printer.set_current_genotypes(genotypes);
			printer.set_phasing(phasing);
			printer.output_variant(std::cout, first_var);
		}
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
	vcf_input.handle().open(args_info.input_arg);
	
	vcf::reader reader(vcf_input);
	merge::variant_printer variant_printer;
	
	vcf::add_reserved_info_keys(reader.info_fields());
	vcf::add_reserved_genotype_keys(reader.genotype_fields());
	
	variant_list variants_in_current_position, empty_variant_list;
	
	// Read the headers.
	reader.set_variant_format(new variant_format());
	reader.read_header();
	reader.set_parsed_fields(vcf::field::ALL);
	
	// Set up filtering.
	auto const &info_fields(reader.info_fields());
	std::vector <vcf::info_field_base const *> filtered_info_fields;
	filtered_info_fields.reserve(args_info.filter_fields_set_given);
	for (std::size_t i(0); i < args_info.filter_fields_set_given; ++i)
	{
		std::string_view const field_name(args_info.filter_fields_set_arg[i]);
		auto const it(info_fields.find(field_name));
		if (info_fields.end() == it)
		{
			std::cerr << "WARNING: No INFO field with identifier ‘’" << field_name << "’ found in headers.\n";
			continue;
		}
		
		filtered_info_fields.push_back(it->second.get());
	}
	
	// Parse and output.
	output_header(reader);
	bool should_continue(false);
	std::size_t lineno{};
	std::size_t prev_pos{};
	reader.parse_nc(
		[
			&variant_printer,
			&variants_in_current_position,
			&empty_variant_list,
			&reader,
			&filtered_info_fields,
			&lineno,
			&prev_pos
		](vcf::transient_variant &var){
		
			++lineno;
			for (auto const *info_field : filtered_info_fields)
			{
				if (info_field->has_value(var))
					return true;
			}
			
			auto const current_pos(var.pos());
			if (prev_pos != current_pos)
			{
				handle_variants(variants_in_current_position, variant_printer);
				empty_variant_list.splice(empty_variant_list.end(), variants_in_current_position);
			}
			prev_pos = current_pos;
			
			// Make sure that there is an empty variant available.
			if (empty_variant_list.empty())
				empty_variant_list.emplace_front(reader.make_empty_variant());
			
			// Copy.
			empty_variant_list.front() = var;
			{
				auto end_it(empty_variant_list.begin());
				++end_it;
				variants_in_current_position.splice(
					variants_in_current_position.end(),
					empty_variant_list,
					empty_variant_list.begin(),
					end_it
				);
			}
			
			if (0 == lineno % 100000)
				std::cerr << "Handled " << lineno << " lines…\n";
			
			return true;
		}
	);
	handle_variants(variants_in_current_position, variant_printer);
	
	return EXIT_SUCCESS;
}

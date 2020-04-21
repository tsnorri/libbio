/*
 * Copyright (c) 2020 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <filesystem>
#include <fstream>
#include <range/v3/all.hpp>
#include <set>
#include <vector>
#include "cmdline.h"
#include "metadata_checker.hh"
#include "variant_printer.hh"
#include "vcf_input.hh"
#include "vcf_record_generator.hh"


namespace fs	= std::filesystem;
namespace lb	= libbio;
namespace merge	= libbio::vcfmerge;
namespace rsv	= ranges::view;
namespace vcf	= libbio::vcf;


namespace {
	
	struct sourced_variant
	{
		vcf::variant	variant;
		std::size_t		input_index{};
		
		sourced_variant() = default;
		
		sourced_variant(vcf::variant &&variant_, std::size_t const input_index_):
			variant(std::move(variant_)),
			input_index(input_index_)
		{
		}
	};
	
	
	bool operator<(sourced_variant const &lhs, sourced_variant const &rhs)
	{
		return lhs.variant.pos() < rhs.variant.pos();
	}
	
	
	// Generate a sample name for each of the given inputs.
	void make_sample_names(std::vector <merge::vcf_input> const &inputs, std::vector <std::string> &out_sample_names)
	{
		for (auto const &input : inputs)
		{
			fs::path path(input.source_path);
			out_sample_names.push_back(path.stem());
		}
	}


	void read_inputs(std::vector <merge::vcf_input> &inputs, char **names, std::size_t const count)
	{
		inputs.resize(count);
		for (std::size_t i(0); i < count; ++i)
		{
			try
			{
				inputs[i].open_file(names[i]);
			}
			catch (std::runtime_error &exc)
			{
				std::cerr << "Unable to open file " << names[i] << ": " << exc.what() << '\n';
				std::exit(EXIT_FAILURE);
			}
		}
	}
	
	
	void read_input_list(std::vector <merge::vcf_input> &inputs, char const *list_path)
	{
		std::vector <std::string> names;
		std::ifstream list_file(list_path);
		std::string line;
		while (std::getline(list_file, line))
			names.push_back(line);
		
		inputs.resize(names.size());
		for (auto &&[name, input] : rsv::zip(names, inputs))
		{
			try
			{
				input.open_file(std::move(name));
			}
			catch (std::runtime_error &exc)
			{
				std::cerr << "Unable to open file " << name << ": " << exc.what() << '\n';
				std::exit(EXIT_FAILURE);
			}
		}
	}
}


int main(int argc, char **argv)
{
	gengetopt_args_info args_info;
	if (0 != cmdline_parser(argc, argv, &args_info))
		std::exit(EXIT_FAILURE);
	
	if (args_info.sample_ploidy_arg < 1)
	{
		std::cerr << "Sample ploidy must be at least one.\n";
		std::exit(EXIT_FAILURE);
	}
	auto const sample_ploidy(args_info.sample_ploidy_arg);
	
	std::ios_base::sync_with_stdio(false);	// Don't use C style IO after calling cmdline_parser.
	
	// Open the VCF files.
	std::vector <merge::vcf_input> inputs;
	if (args_info.input_list_given)
		read_input_list(inputs, args_info.input_list_arg);
	else
		read_inputs(inputs, args_info.input_arg, args_info.input_given);
	
	// Read the headers.
	for (auto &input : inputs)
	{
		input.reader.fill_buffer();
		input.reader.read_header();
	}
	
	// Check that the metadata are compatible.
	merge::metadata_checker <merge::compare_alt>	alt_checker;
	merge::metadata_checker <merge::compare_filter>	filter_checker;
	merge::metadata_checker <merge::compare_contig>	contig_checker;
	merge::metadata_checker <merge::compare_info>	info_checker;
	merge::metadata_checker <merge::compare_format>	format_checker;
	
	std::vector <merge::metadata_sorter_base *> const sorters{
		&alt_checker,
		&filter_checker,
		&contig_checker,
		&info_checker,
		&format_checker
	};
	
	std::vector <merge::metadata_checker_base *> const checkers{
		&alt_checker,
		&filter_checker,
		&contig_checker,
		&info_checker,
		&format_checker
	};
	
	for (auto *sorter : sorters)
		sorter->sort_by_key(inputs);
	
	bool metadata_are_compatible(true);
	for (auto *checker : checkers)
		metadata_are_compatible &= checker->check_metadata_required_matches(inputs);
	
	if (!metadata_are_compatible)
		return EXIT_FAILURE;
	
	// Output the headers.
	std::cout << "##fileformat=VCFv4.3\n";
	for (auto const *sorter : sorters)
		sorter->output(std::cout);
	
	// Output all assembly headers.
	for (auto const &input : inputs)
	{
		for (auto const &assembly : input.reader.metadata().assembly())
			assembly.output_vcf(std::cout);
	}
	
	// Merge the records and output.
	if (args_info.generate_samples_given)
	{
		std::vector <std::string> sample_names;
		make_sample_names(inputs, sample_names);
		
		{
			auto const &format_meta(format_checker.metadata_by_key());
			if (format_meta.end() == format_meta.find("GT"))
				std::cout << "##FORMAT=<ID=GT,Number=1,Type=String,Description=\"Genotype\">\n";
		}
		
		// Output the column headers.
		std::cout << "#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\tFORMAT";
		for (auto const &sample_name : sample_names)
			std::cout << '\t' << sample_name;
		std::cout << '\n';
		
		// Output the records in order.
		// Use a multiset in order to reuse the variants’ buffers.
		std::multiset <sourced_variant> variants;
		for (auto const &[idx, input] : rsv::enumerate(inputs))
		{
			input.reader.set_parsed_fields(vcf::field::INFO);
			
			auto var(input.reader.make_empty_variant());
			if (input.record_generator.get_next_variant(var))
				variants.emplace(std::move(var), idx);
		}
		
		merge::variant_printer <vcf::variant> printer(inputs.size(), sample_ploidy, args_info.samples_are_phased_flag);
		while (!variants.empty())
		{
			// Get the next vairant.
			auto node(variants.extract(variants.begin()));
			auto &sourced_var(node.value());
			auto const idx(sourced_var.input_index);
			libbio_assert_lt(idx, inputs.size());
			auto &input(inputs[idx]);
			
			printer.set_active_sample_index(idx);
			printer.output_variant(std::cout, sourced_var.variant);
			
			if (input.record_generator.get_next_variant(sourced_var.variant))
				variants.insert(sourced_var);
		}
	}
	else
	{
		// TODO: Merge the sample names.
		std::cerr << "Not implemented.\n";
		return EXIT_FAILURE;
	}
	
	return EXIT_SUCCESS;
}

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
	
	typedef std::vector <merge::vcf_input>	vcf_input_vector;
	
	
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
	
	typedef std::multiset <sourced_variant>	variant_set;
	
	bool operator<(sourced_variant const &lhs, sourced_variant const &rhs)
	{
		return lhs.variant.pos() < rhs.variant.pos();
	}
	
	
	void read_inputs(vcf_input_vector &inputs, char **names, std::size_t const count)
	{
		inputs.resize(count);
		for (std::size_t i(0); i < count; ++i)
		{
			try
			{
				inputs[i].open_file(names[i]);
			}
			catch (std::runtime_error const &exc)
			{
				std::cerr << "Unable to open file " << inputs[i].source_path << ": " << exc.what() << '\n';
				std::exit(EXIT_FAILURE);
			}
		}
	}
	
	
	// Read inputs from a list file.
	void read_input_list(vcf_input_vector &inputs, char const *list_path)
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
			catch (std::runtime_error const &exc)
			{
				std::cerr << "Unable to open file " << name << ": " << exc.what() << '\n';
				std::exit(EXIT_FAILURE);
			}
		}
	}
	
	
	struct merge_strategy
	{
		typedef std::vector <std::string>	sample_name_vector;
		
		virtual ~merge_strategy() {}
		virtual void make_sample_names(vcf_input_vector const &inputs, sample_name_vector &sample_names) const = 0;
		virtual void prepare_variants(variant_set &variants, vcf_input_vector &inputs) = 0;
		virtual void output_variant(std::size_t const input_idx, vcf::variant const &variant) = 0;
		
		void output_column_header(vcf_input_vector const &inputs) const
		{
			// Determine the sample names.
			sample_name_vector sample_names;
			make_sample_names(inputs, sample_names);
			
			// Output the column headers.
			std::cout << "#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\tFORMAT";
			for (auto const &sample_name : sample_names)
				std::cout << '\t' << sample_name;
			std::cout << '\n';
		}
		
		void output_variants(variant_set &variants, vcf_input_vector &inputs)
		{
			while (!variants.empty())
			{
				// Get the next variant.
				auto node(variants.extract(variants.begin()));
				auto &sourced_var(node.value());
				auto const idx(sourced_var.input_index);
				libbio_assert_lt(idx, inputs.size());
				auto &input(inputs[idx]);
				
				output_variant(idx, sourced_var.variant);
				
				if (input.record_generator.get_next_variant(sourced_var.variant))
					variants.insert(sourced_var);
			}
		}
		
		virtual void process_variants(vcf_input_vector &inputs)
		{
			// Output the records in order.
			// Use a multiset in order to reuse the variants’ buffers and prepare it with the first variant from each input.
			variant_set variants;
			output_column_header(inputs);
			prepare_variants(variants, inputs);
			output_variants(variants, inputs);
		}
	};
	
	
	class merge_gs_strategy final : public merge_strategy
	{
	protected:
		merge::variant_printer_gs <vcf::variant>	m_printer;
		
	public:
		merge_gs_strategy(std::size_t const input_count, std::size_t const sample_ploidy, bool const samples_are_phased):
			m_printer(input_count, sample_ploidy, samples_are_phased)
		{
		}
		
		// Generate a sample name for each of the given inputs.
		void make_sample_names(vcf_input_vector const &inputs, sample_name_vector &out_sample_names) const override
		{
			for (auto const &input : inputs)
			{
				fs::path path(input.source_path);
				out_sample_names.push_back(path.stem());
			}
		}
		
		void prepare_variants(variant_set &variants, vcf_input_vector &inputs) override
		{
			for (auto const &[idx, input] : rsv::enumerate(inputs))
			{
				input.reader.set_parsed_fields(vcf::field::INFO);

				auto var(input.reader.make_empty_variant());
				if (input.record_generator.get_next_variant(var))
					variants.emplace(std::move(var), idx);
			}
		}
		
		void output_variant(std::size_t const input_idx, vcf::variant const &variant) override
		{
			m_printer.set_active_sample_index(input_idx);
			m_printer.output_variant(std::cout, variant);
		}
	};
	
	
	class merge_ms_strategy final : public merge_strategy
	{
	protected:
		merge::variant_printer_ms <vcf::variant>	m_printer;
		std::vector <std::size_t>					m_sample_count_csum;
		bool										m_should_merge_sample_names{};
		
	public:
		merge_ms_strategy(
			std::size_t const input_count,
			std::size_t const sample_ploidy,
			bool const samples_are_phased,
			bool const should_merge_sample_names
		):
			m_printer(0, sample_ploidy, samples_are_phased), // Total samples not yet known.
			m_sample_count_csum(1 + input_count, 0),
			m_should_merge_sample_names(should_merge_sample_names)
		{
		}
		
		// Generate a sample name for each of the samples in the given inputs.
		void make_sample_names(vcf_input_vector const &inputs, sample_name_vector &out_sample_names) const override
		{
			if (m_should_merge_sample_names)
			{
				// Count the total samples.
				std::size_t total_samples(0);
				for (auto const &input : inputs)
					total_samples += input.reader.sample_count();
				
				out_sample_names.resize(total_samples);
				
				// Read the sample names and verify that they are unique.
				std::set <std::string> all_sample_names;
				std::size_t out_start_idx(0);
				for (auto const &input : inputs)
				{
					auto const &reader(input.reader);
					auto const &sample_names(reader.sample_indices_by_name());
					
					for (auto const &[sample_name, sample_idx] : sample_names)
					{
						// Check that the name is distinct.
						auto const res(all_sample_names.emplace(sample_name));
						if (!res.second)
						{
							std::cerr << "ERROR: Duplicate sample name “" << sample_name << "”\n";
							std::exit(EXIT_FAILURE);
						}
						
						// Write to the output.
						libbio_assert(sample_idx);
						auto const out_idx(out_start_idx + sample_idx - 1);
						libbio_assert_lt(out_idx, out_sample_names.size());
						out_sample_names[out_idx] = sample_name;
					}
					
					out_start_idx += sample_names.size();
				}
			}
			else
			{
				std::stringstream buffer;
				for (auto const &[input_idx, input] : rsv::enumerate(inputs))
				{
					
					for (auto const sample_idx : rsv::iota(std::size_t(0), input.reader.sample_count()))
					{
						buffer.str(std::string()); // Clear the buffer.
						buffer << "SAMPLE" << (1 + input_idx) << '.' << (1 + sample_idx);
						out_sample_names.emplace_back(buffer.str());
					}
				}
			}
		}
		
		void prepare_variants(variant_set &variants, vcf_input_vector &inputs) override
		{
			// Prepare a cumulative sum of the sample counts.
			std::size_t sample_count_csum(0);
			for (auto const &[idx, input] : rsv::enumerate(inputs))
			{
				auto const sample_count(input.reader.sample_count());
				if (sample_count)
				{
					input.reader.set_parsed_fields(vcf::field::ALL);
					
					auto var(input.reader.make_empty_variant());
					if (input.record_generator.get_next_variant(var))
						variants.emplace(std::move(var), sample_count_csum);
					
					sample_count_csum += sample_count;
				}
				
				auto const csidx(1 + idx);
				libbio_assert_lt(csidx, m_sample_count_csum.size());
				m_sample_count_csum[csidx] = sample_count_csum;
			}
			
			// Update the total sample count.
			m_printer.set_total_samples(sample_count_csum);
		}
		
		void output_variant(std::size_t const input_idx, vcf::variant const &variant) override
		{
			libbio_assert_lt(1 + input_idx, m_sample_count_csum.size());
			auto const lb(m_sample_count_csum[input_idx]);
			auto const rb(m_sample_count_csum[1 + input_idx]);
			m_printer.set_active_sample_range(lb, rb);
			m_printer.output_variant(std::cout, variant);
		}
	};
}


int main(int argc, char **argv)
{
	gengetopt_args_info args_info;
	if (0 != cmdline_parser(argc, argv, &args_info))
		std::exit(EXIT_FAILURE);
	
	// FIXME: Currently we require that the user sets the sample ploidy even when merging samples. To determine the ploidy from the inputs, we would have to store the ploidy for each sample by reading the first variant record of each input.
	if (args_info.sample_ploidy_arg < 1)
	{
		std::cerr << "Sample ploidy must be at least one.\n";
		std::exit(EXIT_FAILURE);
	}
	auto const sample_ploidy(args_info.sample_ploidy_arg);
	
	std::ios_base::sync_with_stdio(false);	// Don't use C style IO after calling cmdline_parser.
	
	// Open the VCF files.
	vcf_input_vector inputs;
	if (args_info.input_list_given)
		read_input_list(inputs, args_info.input_list_arg);
	else
		read_inputs(inputs, args_info.input_arg, args_info.input_given);
	
	// Read the headers.
	for (auto &input : inputs)
		input.reader.read_header();
	
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
	
	// Output the format header for GT if it did not exist.
	{
		auto const &format_meta(format_checker.metadata_by_key());
		if (format_meta.end() == format_meta.find("GT"))
			std::cout << "##FORMAT=<ID=GT,Number=1,Type=String,Description=\"Genotype\">\n";
	}
	
	// Merge the records and output.
	// FIXME: Rewrite the ID column contents?
	if (args_info.generate_samples_given)
	{
		merge_gs_strategy strategy(inputs.size(), sample_ploidy, args_info.samples_are_phased_flag);
		strategy.process_variants(inputs);
	}
	else if (args_info.merge_samples_given)
	{
		std::cerr << "NOTE: Merging mixed ploidies (e.g. samples with two X chromosomes and X and Y chromosomes) has not been implemented.\n";
		
		merge_ms_strategy strategy(inputs.size(), sample_ploidy, args_info.samples_are_phased_flag, !args_info.rename_samples_given);
		strategy.process_variants(inputs);
	}
	else
	{
		std::cerr << "Not implemented.\n";
		return EXIT_FAILURE;
	}
	
	return EXIT_SUCCESS;
}

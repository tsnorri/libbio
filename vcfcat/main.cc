/*
 * Copyright (c) 2019 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <libbio/assert.hh>
#include <libbio/file_handling.hh>
#include <libbio/vcf/variant.hh>
#include <libbio/vcf/vcf_reader.hh>
#include <libbio/vcf/vcf_subfield.hh>
#include "cmdline.h"


namespace lb	= libbio;


namespace {
	
	void output_header(lb::vcf_reader const &reader, std::ostream &stream)
	{
		auto const &metadata(reader.metadata());
		
		stream << "##fileformat=VCFv4.3\n";
		metadata.visit_all_metadata([&stream](lb::vcf_metadata_base const &meta){
			meta.output_vcf(stream);
		});
		
		std::vector <std::string const *> sample_names(reader.sample_count());
		for (auto const &[name, number] : reader.sample_names())
		{
			libbio_assert_lt(0, number);
			sample_names[number - 1] = &name;
		}
		
		stream << "#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\tFORMAT";
		for (auto const ptr : sample_names)
			stream << '\t' << *ptr;
		stream << '\n';
	}


	void output_vcf(lb::vcf_reader &reader, std::ostream &stream)
	{
		output_header(reader, stream);
		
		bool should_continue(false);
		do {
			reader.fill_buffer();
			should_continue = reader.parse([&stream](lb::transient_variant const &var){
				lb::output_vcf(stream, var);
				stream << '\n';
				return true;
			});
		} while (should_continue);
	}
}


int main(int argc, char **argv)
{
	gengetopt_args_info args_info;
	if (0 != cmdline_parser(argc, argv, &args_info))
		exit(EXIT_FAILURE);
	
	std::ios_base::sync_with_stdio(false);	// Don't use C style IO after calling cmdline_parser.
	std::cin.tie(nullptr);					// We don't require any input from the user.
	
	// Open the variant file.
	lb::mmap_handle <char> input_handle;
	lb::file_ostream output_stream;
	
	input_handle.open(args_info.input_arg);
	if (args_info.output_given)
		lb::open_file_for_writing(args_info.output_arg, output_stream, lb::writing_open_mode::CREATE);
	
	// Create a VCF input.
	lb::vcf_mmap_input vcf_input(input_handle);
	
	// Create the parser and add the fields listed in the specification to the metadata.
	lb::vcf_reader reader;
	lb::add_reserved_info_keys(reader.info_fields());
	lb::add_reserved_genotype_keys(reader.genotype_fields());
	
	// Parse.
	reader.set_parsed_fields(lb::vcf_field::ALL);
	reader.set_input(vcf_input);
	reader.fill_buffer();
	reader.read_header();
	output_vcf(reader, args_info.output_given ? output_stream : std::cout);
	
	return EXIT_SUCCESS;
}

/*
 * Copyright (c) 2022 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <libbio/assert.hh>
#include <libbio/cxxcompat.hh>
#include <libbio/file_handling.hh>
#include <libbio/vcf/variant.hh>
#include <libbio/vcf/vcf_reader.hh>
#include <string>
#include <string_view>
#include <vector>
#include "cmdline.h"


namespace lb	= libbio;
namespace vcf	= libbio::vcf;

namespace {
	
	void check_ref(vcf::reader &reader, std::vector <char> const &reference, char const *expected_chr_id)
	{
		std::size_t matches{};
		std::size_t mismatches{};
		reader.parse(
			[
				&reader,
				&reference,
				&matches,
				&mismatches,
				expected_chr_id	// Pointer
			](vcf::transient_variant const &var){
				
				if (var.chrom_id() != expected_chr_id)
				{
					++mismatches;
					return true;
				}
				
				++matches;
				auto const pos(var.zero_based_pos());
				auto const &ref(var.ref());
				std::string_view const expected_ref(reference.data() + pos, ref.size());
				if (ref != expected_ref)
				{
					std::cerr
						<< "WARNING: Variant on line "
						<< var.lineno()
						<< " has REF column value “"
						<< ref
						<< "” but the reference contains “"
						<< expected_ref
						<< "”.\n";
				}
				
				return true;
			}
		);
		
		lb::log_time(std::cerr) << "Done. Chromosome ID matches: " << matches << " mismatches: " << mismatches << ".\n";
	}
	
	
	void read_reference(char const *path, std::vector <char> &dst)
	{
		// FIXME: Handle FASTA in addition to text without terminating newline.
		// FIXME: Handle compressed input.
		
		dst.clear();
		
		lb::file_istream stream;
		lb::open_file_for_reading(path, stream);
		
		// Check the file size.
		struct stat sb;
		auto const res(::fstat(stream->handle(), &sb));
		if (0 != res)
		{
			std::cerr << strerror(errno) << '\n';
			std::exit(EXIT_FAILURE);
		}
		
		dst.reserve(sb.st_size);
		
		// FIXME: likely inefficient.
		char cc{};
		while (stream >> std::skipws >> cc)
		{
			switch (cc)
			{
				case '-':
					break;
				
				default:
					dst.push_back(cc);
					break;
			}
		}
		
		dst.shrink_to_fit();
	}
}


int main(int argc, char **argv)
{
	gengetopt_args_info args_info;
	if (0 != cmdline_parser(argc, argv, &args_info))
		std::exit(EXIT_FAILURE);
	
	std::ios_base::sync_with_stdio(false);	// Don't use C style IO after calling cmdline_parser.
	std::cin.tie(nullptr);					// We don't require any input from the user.
	
	// Open the variant file.
	// FIXME: use stream input, handle compressed input.
	vcf::mmap_input vcf_input;
	vcf_input.handle().open(args_info.variants_arg);
	
	std::vector <char> reference;
	read_reference(args_info.reference_arg, reference);
	
	// Instantiate the parser and add the fields listed in the specification to the metadata.
	vcf::reader reader;
	vcf::add_reserved_info_keys(reader.info_fields());
	vcf::add_reserved_genotype_keys(reader.genotype_fields());
	
	// Parse.
	reader.set_input(vcf_input);
	reader.read_header();
	reader.set_parsed_fields(vcf::field::REF);
	
	check_ref(reader, reference, args_info.chromosome_arg);
	
	return EXIT_SUCCESS;
}

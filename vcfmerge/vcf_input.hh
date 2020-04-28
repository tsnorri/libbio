/*
 * Copyright (c) 2020 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCFMERGE_VCF_INPUT_HH
#define LIBBIO_VCFMERGE_VCF_INPUT_HH

#include <libbio/vcf/vcf_input.hh>
#include <libbio/vcf/vcf_reader.hh>
#include <string>
#include "vcf_record_generator.hh"


namespace libbio::vcfmerge {
	
	struct vcf_input
	{
		libbio::vcf::mmap_input		input;
		libbio::vcf::reader			reader;
		vcf_record_generator		record_generator;
		std::string					source_path;
		
		vcf_input():
			reader(input),
			record_generator(reader)
		{
		}
		
		void open_file()
		{
			input.handle().open(source_path);
			
			vcf::add_reserved_info_keys(reader.info_fields());
			vcf::add_reserved_genotype_keys(reader.genotype_fields());
		}
		
		void open_file(char const *path)
		{
			source_path = path;
			open_file();
		}
		
		void open_file(std::string &&path)
		{
			source_path = std::move(path);
			open_file();
		}
	};
}

#endif

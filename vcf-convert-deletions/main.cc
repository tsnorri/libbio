/*
 * Copyright (c) 2021 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <libbio/fasta_reader.hh>
#include <libbio/vcf/metadata.hh>
#include <libbio/vcf/variant_printer.hh>
#include <libbio/vcf/vcf_reader.hh>
#include <list>
#include <range/v3/all.hpp>
#include "cmdline.h"


namespace lb	= libbio;
namespace rsv	= ranges::view;
namespace vcf	= libbio::vcf;


namespace {
	
	class fasta_reader_delegate final : public lb::fasta_reader_delegate
	{
	protected:
		std::string	m_sequence;
		bool		m_is_first{true};
		
	public:
		std::string &sequence() { return m_sequence; }
		
		bool handle_comment_line(lb::fasta_reader &reader, std::string_view const &sv) override { return true; }
		
		bool handle_identifier(lb::fasta_reader &reader, std::string_view const &sv) override
		{
			if (m_is_first)
			{
				std::cerr << "Reading sequence “" << sv << "” from input FASTA…\n";
				m_is_first = false;
				return true;
			}
			else
			{
				return false;
			}
		}
		
		bool handle_sequence_line(lb::fasta_reader &reader, std::string_view const &sv) override
		{
			m_sequence += sv;
			return true;
		}
	};
	
	
	class variant_printer final : public vcf::variant_printer_base <vcf::transient_variant>
	{
	protected:
		std::string const	&m_reference;
		char				m_added_character{};
		bool				m_is_before{};
		
	public:
		variant_printer(std::string const &reference):
			m_reference(reference)
		{
		}
		
		
		void read_added_character(vcf::transient_variant const &var)
		{
			// E.g.
			// 012
			// GAT
			// 123
			//
			// 2	A	<DEL>
			
			auto const pos(var.zero_based_pos());
			auto const &ref(var.ref());
			if (pos)
			{
				m_added_character = m_reference[pos - 1];
				m_is_before = true;
			}
			else // 0 == pos
			{
				auto const ref_len(ref.size());
				m_added_character = m_reference[ref_len];
				m_is_before = false;
			}
		}
		
		
		void output_pos(std::ostream &os, vcf::transient_variant const &var) const override
		{
			auto const pos(var.zero_based_pos());
			if (pos)
				os << pos; // Subtract one.
			else
				os << 1 + pos;
		}
		
		
		void output_ref(std::ostream &os, vcf::transient_variant const &var) const override
		{
			if (m_is_before)
				os << m_added_character << var.ref();
			else
				os << var.ref() << m_added_character;
		}
		
		
		void output_alt(std::ostream &os, vcf::transient_variant const &var, vcf::transient_variant::variant_alt_type const &alt) const
		{
			switch (alt.alt_sv_type)
			{
				case vcf::sv_type::DEL:
					os << m_added_character;
					break;
				
				case vcf::sv_type::NONE:
					if (m_is_before)
						os << m_added_character << alt.alt;
					else
						os << alt.alt << m_added_character;
					break;
					
				default:
					std::cerr << "ERROR: Unexpected ALT type on line " << var.lineno() << '\n';
					std::exit(EXIT_FAILURE);
			}
		}
		
		
		void output_alt(std::ostream &os, vcf::transient_variant const &var) const override
		{
			bool is_first(true);
			for (auto const &alt : var.alts())
			{
				if (!is_first)
					os << ',';
				
				output_alt(os, var, alt);;
				is_first = false;
			}
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
}


int main(int argc, char **argv)
{
	gengetopt_args_info args_info;
	if (0 != cmdline_parser(argc, argv, &args_info))
		std::exit(EXIT_FAILURE);
	
	std::ios_base::sync_with_stdio(false);	// Don't use C style IO after calling cmdline_parser.
	
	// Read the reference sequence.
	std::string reference;
	{
		lb::fasta_reader fasta_reader;
		fasta_reader_delegate delegate;
		lb::mmap_handle <char> handle;
		handle.open(args_info.reference_arg);
		fasta_reader.parse(handle, delegate);
		reference = std::move(delegate.sequence());
	}
	
	// Open the variant file.
	vcf::mmap_input vcf_input;
	vcf_input.handle().open(args_info.variants_arg);
	
	vcf::reader reader(vcf_input);
	
	vcf::add_reserved_info_keys(reader.info_fields());
	vcf::add_reserved_genotype_keys(reader.genotype_fields());
	
	// Read the headers.
	reader.read_header();
	reader.set_parsed_fields(vcf::field::ALL);
	
	// Parse and output.
	output_header(reader);
	variant_printer printer(reference);
	reader.parse(
		[
			&printer
		](vcf::transient_variant const &var){
			
			// Check if any of the ALT is a deletion; in that case use the custom printer.
			for (auto const &alt : var.alts())
			{
				if (vcf::sv_type::DEL == alt.alt_sv_type)
				{
					printer.read_added_character(var);
					printer.output_variant(std::cout, var);
					return true;
				}
			}
			
			// Otherwise print with the default printer.
			vcf::output_vcf(std::cout, var);
			return true;
		}
	);
	
	return EXIT_SUCCESS;
}

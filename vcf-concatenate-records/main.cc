/*
 * Copyright (c) 2021 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <libbio/fasta_reader.hh>
#include <libbio/vcf/metadata.hh>
#include <libbio/vcf/subfield.hh>
#include <libbio/vcf/variant_format.hh>
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
			switch (meta.type())
			{
				// Skip INFO because we set the field contents to undefined.
				case lb::vcf::metadata_type::INFO:
					break;
				
				default:
					meta.output_vcf(std::cout);
					break;
			}
		});
		
		std::cout << "#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\tFORMAT";
		for (auto const &name : reader.sample_names_by_index())
			std::cout << '\t' << name;
		std::cout << '\n';
	}
	
	
	struct variant
	{
		std::string chrom_id;
		std::string	ref;
		std::string	alt;
		std::size_t pos{SIZE_MAX};
		
		bool has_value() const { return SIZE_MAX != pos; }
		
		void clear()
		{
			chrom_id.clear();
			ref.clear();
			alt.clear();
			pos = SIZE_MAX;
		}
	};
	
	
	std::ostream &operator<<(std::ostream &os, variant const &var)
	{
		// CHROM POS ID REF ALT QUAL FILTER INFO FORMAT SAMPLE
		os << var.chrom_id << '\t' << (1 + var.pos) << "\t.\t" << var.ref << '\t' << var.alt << "\t.\tPASS\t.\tGT\t1\n";
		return os;
	}
	
	
	void output_if_needed(variant &variant_buffer, std::size_t const prev_end_pos, std::string const &reference)
	{
		if (variant_buffer.has_value())
		{
			// Check for empty variant.
			if (variant_buffer.alt.empty())
			{
				// Check for position zero.
				if (0 == variant_buffer.pos)
				{
					libbio_assert_lt(prev_end_pos, reference.size());
					auto const nt(reference[prev_end_pos]);
					variant_buffer.ref += nt;
					variant_buffer.alt += nt;
				}
				else
				{
					auto const nt(reference[variant_buffer.pos - 1]);
					variant_buffer.ref.insert(0, 1, nt);
					variant_buffer.alt.insert(0, 1, nt);
					--variant_buffer.pos;
				}
			}
			
			std::cout << variant_buffer;
		}
	}
}


int main(int argc, char **argv)
{
	gengetopt_args_info args_info;
	if (0 != cmdline_parser(argc, argv, &args_info))
		std::exit(EXIT_FAILURE);
	
	std::ios_base::sync_with_stdio(false);	// Don't use C style IO after calling cmdline_parser.
	
	std::cerr << "NOTE: Only haploid samples are handled currently.\n";
	
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
	reader.set_variant_format(new variant_format());
	reader.read_header();
	reader.set_parsed_fields(vcf::field::ALL);
	
	if (1 != reader.sample_names_by_index().size())
	{
		std::cerr << "ERROR: Only files with one sample are handled.\n";
		std::exit(EXIT_FAILURE);
	}

	// Parse and output.
	output_header(reader);
	bool is_first(true);
	variant variant_buffer;
	std::size_t prev_pos{};
	std::size_t prev_end_pos{SIZE_MAX};
	std::size_t overlapping_variants{};
	bool const list_overlapping_variants(args_info.list_overlapping_variants_flag);
	reader.parse_nc(
		[
			&reference,
			&is_first,
			&variant_buffer,
			&prev_pos,
			&prev_end_pos,
			&overlapping_variants,
			list_overlapping_variants
		](vcf::transient_variant const &var){
			
			libbio_assert_eq(1, var.samples().size());
			auto const &sample(var.samples().front());
			auto const &gt_field(*static_cast <variant_format const &>(var.get_format()).gt_field);
			auto const &gt(gt_field(sample));
			
			// Skip zero GT values.
			libbio_always_assert_eq_msg(1, gt.size(), "Only haploid samples are handled");
			if (!gt.front().alt)
				return true;

			libbio_always_assert_eq(1, var.alts().size());
			auto const &alt(var.alts().front());

			// Skip ALT equal to REF.
			if (lb::vcf::sv_type::NONE == alt.alt_sv_type && var.ref() == alt.alt)
				return true;
			
			// Compare to the previous position.
			// FIXME: check chrom_id.
			// FIXME: check the END value, too.
			auto const current_pos(var.zero_based_pos());
			auto const current_end_pos(current_pos + var.ref().size());
			if (SIZE_MAX != prev_end_pos && current_pos < prev_end_pos)
			{
				++overlapping_variants;
				if (list_overlapping_variants)
					std::cerr << "NOTICE: Skipping overlapping variant on line " << var.lineno() <<
					" ([" << prev_pos << ", " << prev_end_pos << ") vs. [" << current_pos << ", " << current_end_pos << ")).\n";
				return true;
			}
			else if (prev_end_pos != current_pos)
			{
				// The previous variant is too far away. Output the buffer contents.
				output_if_needed(variant_buffer, prev_end_pos, reference);
				
				variant_buffer.clear();
				variant_buffer.chrom_id = var.chrom_id();
				variant_buffer.pos = current_pos;
			}
			
			// Append to the buffer.
			variant_buffer.ref += var.ref();
			
			switch (alt.alt_sv_type)
			{
				case lb::vcf::sv_type::NONE:
					variant_buffer.alt += alt.alt;
					break;
				
				case lb::vcf::sv_type::DEL:
					break;
				
				default:
					std::cerr << "ERROR: Unexpected structural variant type “" << lb::to_string(alt.alt_sv_type) << "”.\n";
					std::exit(EXIT_FAILURE);
			}
			
			prev_pos = current_pos;
			prev_end_pos = current_end_pos;
			
			return true;
		}
	);
	
	output_if_needed(variant_buffer, prev_end_pos, reference);

	std::cerr << "Skipped " << overlapping_variants << " variants due to overlaps.\n";
	
	return EXIT_SUCCESS;
}

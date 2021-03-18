/*
 * Copyright (c) 2021 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <boost/gil.hpp>
#include <boost/gil/extension/io/png.hpp>
#include <libbio/utility/misc.hh>
#include <libbio/vcf/vcf_reader.hh>
#include <range/v3/view/enumerate.hpp>
#include "cmdline.h"

namespace lb	= libbio;
namespace gil	= boost::gil;
namespace rsv	= ranges::view;
namespace vcf	= libbio::vcf;


namespace {
	
	// Specify colours for up to seven ALT values and REF.
	// Not constexpr b.c. gil::rgb8_pixel_t in Boost 1.75 does not have a constexpr constructor.
	static inline auto const COLOURS{libbio::make_array <gil::rgb8_pixel_t>(
		gil::rgb8_pixel_t{0xff, 0xff, 0xff},
		gil::rgb8_pixel_t{0x00, 0x00, 0x00},
		gil::rgb8_pixel_t{0xff, 0x00, 0x00},
		gil::rgb8_pixel_t{0x00, 0xff, 0x00},
		gil::rgb8_pixel_t{0x00, 0x00, 0xff},
		gil::rgb8_pixel_t{0xff, 0xff, 0x00},
		gil::rgb8_pixel_t{0xff, 0x00, 0xff},
		gil::rgb8_pixel_t{0x00, 0xff, 0xff}
	)};
	
	
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
	
	
	std::size_t count_variants(vcf::mmap_input &vcf_input)
	{
		// Count the records.
		// Since the reader cannot backtrack, instantiate a new reader.
		vcf::reader reader(vcf_input);
		reader.read_header();
		
		reader.set_parsed_fields(vcf::field::CHROM);
		std::size_t variant_count{};
		reader.parse([&variant_count](vcf::transient_variant const &){
			++variant_count;
			return true;
		});
		
		return variant_count;
	}
}


int main(int argc, char **argv)
{
	gengetopt_args_info args_info;
	if (0 != cmdline_parser(argc, argv, &args_info))
		std::exit(EXIT_FAILURE);
	
	std::ios_base::sync_with_stdio(false);	// Don't use C style IO after calling cmdline_parser.
	
	if (args_info.ploidy_arg <= 0)
	{
		std::cerr << "Ploidy must be non-negative.\n";
		std::exit(EXIT_FAILURE);
	}
	std::uint16_t const ploidy(args_info.ploidy_arg);
	
	// Open the variant file.
	vcf::mmap_input vcf_input;
	vcf_input.handle().open(args_info.variants_arg);
	
	auto const variant_count(count_variants(vcf_input));
	
	vcf::reader reader(vcf_input);
	
	vcf::add_reserved_info_keys(reader.info_fields());
	vcf::add_reserved_genotype_keys(reader.genotype_fields());
		
	// Read the headers.
	reader.set_variant_format(new variant_format());
	reader.read_header();
	
	// FIXME: Check the ploidy from the file. The user should also be able to specify the chromosome identifier.
	
	// Create the output image.
	// FIXME: 4 bytes per pixel would be enough but GIL does not have that.
	gil::rgb8_image_t image(variant_count, ploidy * reader.sample_count());
	auto image_view(gil::view(image));
	boost::gil::fill_pixels(image_view, COLOURS[0]);
	
	// Handle the genotype values.
	std::size_t variant_idx{};
	reader.set_parsed_fields(vcf::field::ALL);
	reader.parse([&image_view, &variant_idx, ploidy](vcf::transient_variant const &var){
		auto const *gt_field(get_variant_format(var).gt_field);
		for (auto const &[sample_idx, sample] : rsv::enumerate(var.samples()))
		{
			auto const &gt((*gt_field)(sample)); // vector of sample_genotype
			libbio_always_assert(gt.size() <= ploidy);
			for (auto const &[chr_idx, sample_gt] : rsv::enumerate(gt))
			{
				auto const row_idx(ploidy * sample_idx + chr_idx);
				libbio_always_assert(sample_gt.alt < COLOURS.size());
				*image_view.at(variant_idx, row_idx) = COLOURS[sample_gt.alt];
			}
		}
		
		++variant_idx;
		return true;
	});
	
	gil::write_view(std::cout, image_view, gil::image_write_info <gil::png_tag>{});
	
	return EXIT_SUCCESS;
}

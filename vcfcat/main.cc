/*
 * Copyright (c) 2019 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <libbio/assert.hh>
#include <libbio/cxxcompat.hh>
#include <libbio/file_handling.hh>
#include <libbio/vcf/variant.hh>
#include <libbio/vcf/variant_printer.hh>
#include <libbio/vcf/vcf_reader.hh>
#include <libbio/vcf/vcf_subfield.hh>
#include <map>
#include "cmdline.h"


namespace lb	= libbio;

typedef std::span <char const *>			sample_name_span;
typedef std::map <std::size_t, std::size_t>	alt_number_map;


namespace {
	
	// From libvcf2multialign.
	// FIXME: come up with a way not to duplicate the code needed for storing field pointers.
	struct variant_format final : public libbio::variant_format
	{
		libbio::vcf_genotype_field_gt	*gt{};
		
		// Return a new empty instance of this class.
		virtual variant_format *new_instance() const override { return new variant_format(); }
		
		virtual void reader_did_update_format(libbio::vcf_reader &reader) override;
	};
	
	
	template <typename t_variant>
	class sample_filtering_variant_printer final : public lb::variant_printer_base <t_variant>
	{
	public:
		typedef t_variant	variant_type;
		
	protected:
		sample_name_span const	*m_sample_names{};
		alt_number_map const	*m_alt_mapping{};
		bool const				m_exclude_samples{};
		
	public:
		sample_filtering_variant_printer() = default;
		
		sample_filtering_variant_printer(sample_name_span const &sample_names, alt_number_map const &alt_mapping, bool const exclude_samples):
			m_sample_names(&sample_names),
			m_alt_mapping(&alt_mapping),
			m_exclude_samples(exclude_samples)
		{
		}
		
		virtual inline void output_alt(std::ostream &os, variant_type const &var) const override;
		virtual inline void output_samples(std::ostream &os, variant_type const &var) const override;
	};
	
	
	void variant_format::reader_did_update_format(lb::vcf_reader &reader)
	{	
		this->assign_field_ptr("GT", gt);
	}
	
	
	inline variant_format const &get_variant_format(libbio::variant const &var)
	{
		libbio_assert(var.reader()->has_assigned_variant_format());
		return static_cast <variant_format const &>(var.get_format());
	}
	
	
	inline variant_format const &get_variant_format(libbio::transient_variant const &var)
	{
		libbio_assert(var.reader()->has_assigned_variant_format());
		return static_cast <variant_format const &>(var.get_format());
	}
	
	
	template <typename t_variant>
	void sample_filtering_variant_printer <t_variant>::output_alt(std::ostream &os, variant_type const &var) const
	{
		auto const &alts(var.alts());
		ranges::copy(
			*m_alt_mapping | ranges::view::transform([&alts](auto const &kv) -> typename variant_type::string_type const & {
				libbio_assert_lt(0, kv.first);
				libbio_assert_lte(kv.first, alts.size());
				return alts[kv.first - 1].alt; 
			}),
			ranges::make_ostream_joiner(os, ",")
		);
	}
	
	
	template <typename t_variant>
	void sample_filtering_variant_printer <t_variant>::output_samples(std::ostream &os, variant_type const &var) const
	{
		auto const &reader(var.reader());
		auto const &parsed_sample_names(reader->sample_names());
		bool is_first(true);
		auto const &samples(var.samples());
		
		auto cb([this, &os, &var, &is_first, &samples](std::size_t const idx1){
			libbio_assert(idx1);
			auto const idx(idx1 - 1);
			auto const &sample(samples[idx]);
			
			if (!is_first)
				os << '\t';
			is_first = false;
			this->output_sample(os, var, sample);
		});
		
		if (m_exclude_samples)
		{
			for (auto const &pair : parsed_sample_names)
			{
				auto const &name(pair.first);
				if (std::find(m_sample_names->begin(), m_sample_names->end(), name) == m_sample_names->end())
				{
					auto const idx1(pair.second);
					cb(idx1);
				}
			}
		}
		else
		{
			for (auto const &name : *m_sample_names)
			{
				auto const idx1(parsed_sample_names.find(name)->second);
				cb(idx1);
			}
		}
	}
	
	
	void check_sample_names(lb::vcf_reader const &reader, sample_name_span const &sample_names)
	{
		// Check whether the given sample names actually exist.
		auto const &parsed_sample_names(reader.sample_names());
		bool can_continue(true);
		for (auto const &name : sample_names)
		{
			if (parsed_sample_names.find(name) == parsed_sample_names.end())
			{
				std::cerr << "ERROR: sample “" << name << "” not present in the given variant file.\n";
				can_continue = false;
			}
		}
		
		if (!can_continue)
			std::exit(EXIT_FAILURE);
	}
	
	
	bool modify_variant(lb::transient_variant &var, alt_number_map &alt_mapping, sample_name_span const &sample_names, bool const exclude_samples)
	{
		alt_mapping.clear();
		
		auto const *gt_field(get_variant_format(var).gt);
		auto const &reader(*var.reader());
		auto const &parsed_sample_names(reader.sample_names());
		auto &samples(var.samples());
		
		// Find the ALT values that are in use in the given samples.
		{
			auto const cb([&samples, &gt_field, &alt_mapping](std::size_t const idx1){
				libbio_assert(idx1);
				auto const idx(idx1 - 1);
				auto &sample(samples[idx]);
				for (auto const &gt : (*gt_field)(sample))
					alt_mapping.emplace(gt.alt, 0);
			});
			
			if (exclude_samples)
			{
				for (auto const &pair : parsed_sample_names)
				{
					auto const &name(pair.first);
					auto const it(std::find(sample_names.begin(), sample_names.end(), name));
					// Check whether the current sample name is also in the excluded list.
					if (it != sample_names.end())
						continue;
					
					auto const idx1(pair.second);
					cb(idx1);
				}
			}
			else
			{
				for (auto const &name : sample_names)
				{
					auto const it(parsed_sample_names.find(name));
					libbio_assert_neq(it, parsed_sample_names.end());
					
					auto const idx1(it->second);
					cb(idx1);
				}
			}
			
			alt_mapping.erase(0);
			if (alt_mapping.empty())
				return false;
		}
		
		{
			std::size_t idx{};
			for (auto &kv : alt_mapping)
				kv.second = ++idx;
		}
		
		// Change the GT values.
		for (auto const &name : sample_names)
		{
			auto const it(parsed_sample_names.find(name));
			libbio_assert_neq(it, parsed_sample_names.end());
			auto const idx1(it->second);
			libbio_assert(idx1);
			auto const idx(idx1 - 1);
			auto &sample(samples[idx]);
			
			for (auto &gt : (*gt_field)(sample))
			{
				if (gt.alt)
					gt.alt = alt_mapping[gt.alt];
			}
		}
		
		return true;
	}
	
	
	void output_header(lb::vcf_reader const &reader, std::ostream &stream, sample_name_span const &sample_names, bool const exclude_samples)
	{
		auto const &metadata(reader.metadata());
		
		stream << "##fileformat=VCFv4.3\n";
		metadata.visit_all_metadata([&stream](lb::vcf_metadata_base const &meta){
			meta.output_vcf(stream);
		});
		
		stream << "#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\tFORMAT";
		if (sample_names.empty() || exclude_samples)
		{
			std::vector <std::string const *> output_sample_names(reader.sample_count());
			std::size_t found_count{};
			for (auto const &[name, number] : reader.sample_names())
			{
				libbio_assert_lt(0, number);

				auto const did_find(std::find(sample_names.begin(), sample_names.end(), name) != sample_names.end());
				if (exclude_samples == did_find)
					continue;

				output_sample_names[found_count] = &name;
				++found_count;
			}
			output_sample_names.resize(found_count);
			
			for (auto const ptr : output_sample_names)
				stream << '\t' << *ptr;
		}
		else
		{
			for (auto const &name : sample_names)
				stream << '\t' << name;
		}
		stream << '\n';
	}
	
	
	void output_vcf(lb::vcf_reader &reader, std::ostream &stream, sample_name_span const &sample_names, bool const exclude_samples)
	{
		if (!sample_names.empty())
			check_sample_names(reader, sample_names);
		
		output_header(reader, stream, sample_names, exclude_samples);
		
		bool should_continue(false);
		std::size_t lineno{};
		alt_number_map alt_mapping;
		do {
			reader.fill_buffer();
			should_continue = reader.parse_nc([&stream, &sample_names, exclude_samples, &alt_mapping, &lineno](lb::transient_variant &var){

				++lineno;
				
				if ((!sample_names.empty()) || exclude_samples)
				{
					if (modify_variant(var, alt_mapping, sample_names, exclude_samples))
					{
						sample_filtering_variant_printer <lb::transient_variant> printer(sample_names, alt_mapping, exclude_samples);
						lb::output_vcf(printer, stream, var);
					}
				}
				else
				{
					lb::output_vcf(stream, var);
				}

				if (0 == lineno % 100000)
					std::cerr << "Handled " << lineno << " lines…\n";
				
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
	
	sample_name_span const sample_names(const_cast <char const **>(args_info.sample_arg), args_info.sample_given);
	
	// Create a VCF input.
	lb::vcf_mmap_input vcf_input(input_handle);
	
	// Create the parser and add the fields listed in the specification to the metadata.
	lb::vcf_reader reader;
	lb::add_reserved_info_keys(reader.info_fields());
	lb::add_reserved_genotype_keys(reader.genotype_fields());
	
	// Parse.
	reader.set_variant_format(new variant_format());
	reader.set_input(vcf_input);
	reader.fill_buffer();
	reader.read_header();
	reader.set_parsed_fields(lb::vcf_field::ALL);
	output_vcf(reader, args_info.output_given ? output_stream : std::cout, sample_names, args_info.exclude_samples_given);
	
	return EXIT_SUCCESS;
}

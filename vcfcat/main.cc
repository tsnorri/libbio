/*
 * Copyright (c) 2019-2022 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <libbio/assert.hh>
#include <libbio/cxxcompat.hh>
#include <libbio/file_handling.hh>
#include <libbio/vcf/subfield.hh>
#include <libbio/vcf/variant.hh>
#include <libbio/vcf/variant_printer.hh>
#include <libbio/vcf/vcf_reader.hh>
#include <map>
#include <numeric>
#include <range/v3/view/take_while.hpp>
#include "cmdline.h"


namespace lb	= libbio;
namespace rsv	= ranges::views;
namespace vcf	= libbio::vcf;

typedef std::vector <std::string>			sample_name_vector;
typedef std::map <std::size_t, std::size_t>	alt_number_map;


namespace {
	
	// From libvcf2multialign.
	// FIXME: come up with a way not to duplicate the code needed for storing field pointers.
	struct variant_format final : public vcf::variant_format
	{
		vcf::genotype_field_gt	*gt{};
		
		// Return a new empty instance of this class.
		virtual variant_format *new_instance() const override { return new variant_format(); }
		
		virtual void reader_did_update_format(vcf::reader &reader) override;
	};
	
	
	class variant_printer_base : public vcf::variant_printer_base <vcf::transient_variant>
	{
		typedef vcf::variant_printer_base <vcf::transient_variant>	base_class;
		
	public:
		using base_class::output_info;
		using base_class::output_format;
		using base_class::output_sample;

		virtual ~variant_printer_base() {}
		virtual void prepare(vcf::reader &reader) {}
		virtual void begin_variant(vcf::reader &reader) {}
		virtual void end_variant(vcf::reader &reader) {}
	};
	
	
	class order_preserving_variant_printer_base : public variant_printer_base
	{
	protected:
		vcf::info_field_ptr_vector		m_info_fields;			// Non-owning.
		vcf::genotype_ptr_vector const	*m_genotype_fields{};	// Non-owning.

	public:
		using variant_printer_base::output_info;
		using variant_printer_base::output_format;
		using variant_printer_base::output_sample;

		void output_info(std::ostream &os, variant_type const &var) const override;
		void output_format(std::ostream &os, variant_type const &var) const override;
		void output_sample(std::ostream &os, variant_type const &var, variant_sample_type const &sample) const override;

		void prepare(vcf::reader &reader) override;
		void begin_variant(vcf::reader &reader) override;
		void end_variant(vcf::reader &reader) override;
	};
	
	
	template <typename t_base>
	class variant_printer final : public t_base
	{
	public:
		typedef typename t_base::variant_type	variant_type;

	public:
		using t_base::t_base;
	};
	
	
	template <typename t_base>
	class sample_filtering_variant_printer final : public t_base
	{
	public:
		typedef typename t_base::variant_type	variant_type;
		
	protected:
		sample_name_vector const	*m_sample_names{};
		alt_number_map const		*m_alt_mapping{};
		bool const					m_exclude_samples{};
		
	public:
		sample_filtering_variant_printer() = default;
		
		sample_filtering_variant_printer(sample_name_vector const &sample_names, alt_number_map const &alt_mapping, bool const exclude_samples):
			m_sample_names(&sample_names),
			m_alt_mapping(&alt_mapping),
			m_exclude_samples(exclude_samples)
		{
		}
		
		void output_alt(std::ostream &os, variant_type const &var) const override;
		void output_samples(std::ostream &os, variant_type const &var) const override;
	};
	
	
	void variant_format::reader_did_update_format(vcf::reader &reader)
	{	
		this->assign_field_ptr("GT", gt);
	}
	
	
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
	
	
	void order_preserving_variant_printer_base::prepare(vcf::reader &reader)
	{
		auto const &info_fields(reader.info_fields_in_headers());
		m_info_fields.clear();
		m_info_fields.resize(info_fields.size(), nullptr);
	}
	
	
	void order_preserving_variant_printer_base::begin_variant(vcf::reader &reader)
	{
		m_genotype_fields = &reader.get_current_variant_format();
		
		auto const &info_fields(reader.info_fields_in_headers());
		m_info_fields.clear();
		m_info_fields.resize(info_fields.size(), nullptr);
		
		for (auto const info_field_ptr : info_fields)
		{
			auto const info_meta_ptr(info_field_ptr->get_metadata());
			auto const idx(info_meta_ptr->get_record_index());
			if (UINT16_MAX == idx)
				continue;
			
			m_info_fields[idx] = info_field_ptr;
		}
	}


	void order_preserving_variant_printer_base::end_variant(vcf::reader &reader)
	{
		for (auto const field_ptr : reader.info_fields_in_headers())
			field_ptr->get_metadata()->reset_record_index();
	}
	
	
	void order_preserving_variant_printer_base::output_info(std::ostream &os, variant_type const &var) const
	{
		output_info(
			os,
			var,
			m_info_fields
			| rsv::take_while([](auto const ptr){ return ptr != nullptr; })
			| rsv::indirect
		);
	}
	
	
	void order_preserving_variant_printer_base::output_format(std::ostream &os, variant_type const &var) const
	{
		output_format(os, var, *m_genotype_fields | rsv::indirect);
	}


	void order_preserving_variant_printer_base::output_sample(
		std::ostream &os,
		variant_type const &var,
		variant_sample_type const &sample
	) const
	{
		output_sample(os, var, sample, *m_genotype_fields | rsv::indirect);
	}
	
	
	template <typename t_variant>
	void sample_filtering_variant_printer <t_variant>::output_alt(std::ostream &os, variant_type const &var) const
	{
		auto const &alts(var.alts());
		ranges::copy(
			*m_alt_mapping
			| ranges::view::transform([&var, &alts](auto const &kv) -> typename variant_type::string_type const & {
				libbio_assert_lt(0, kv.first);
				libbio_assert_lte_msg(kv.first, alts.size(), "lineno: ", var.lineno(), " kv.first: ", kv.first, " alts.size(): ", alts.size());
				return alts[kv.first - 1].alt; 
			}),
			ranges::make_ostream_joiner(os, ",")
		);
	}
	
	
	template <typename t_variant>
	void sample_filtering_variant_printer <t_variant>::output_samples(std::ostream &os, variant_type const &var) const
	{
		auto const &reader(var.reader());
		auto const &parsed_sample_names(reader->sample_indices_by_name());
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
				if (!std::binary_search(m_sample_names->begin(), m_sample_names->end(), name))
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
				auto const it(parsed_sample_names.find(name));
				auto const idx1(parsed_sample_names.find(name)->second);
				cb(idx1);
			}
		}
	}
	
	
	void check_sample_names(vcf::reader const &reader, sample_name_vector const &sample_names)
	{
		// Check whether the given sample names actually exist.
		auto const &parsed_sample_names(reader.sample_indices_by_name());
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
	
	
	bool modify_variant(vcf::transient_variant &var, alt_number_map &alt_mapping, sample_name_vector const &sample_names, bool const exclude_samples)
	{
		alt_mapping.clear();

		auto &samples(var.samples());
		if (samples.empty())
			return true;

		auto const *gt_field(get_variant_format(var).gt);
		auto const &reader(*var.reader());
		auto const &parsed_sample_names(reader.sample_indices_by_name());
		
		// Find the ALT values that are in use in the given samples.
		{
			auto const cb([&samples, &gt_field, &alt_mapping](std::size_t const idx1){
				libbio_assert(idx1);
				auto const idx(idx1 - 1);
				auto &sample(samples[idx]);
				for (auto const &gt : (*gt_field)(sample))
				{
					if (vcf::sample_genotype::NULL_ALLELE == gt.alt || 0 == gt.alt)
						continue;
					alt_mapping.emplace(gt.alt, 0);
				}
			});
			
			if (exclude_samples)
			{
				for (auto const &pair : parsed_sample_names)
				{
					auto const &name(pair.first);
					// Check whether the current sample name is also in the excluded list.
					if (std::binary_search(sample_names.begin(), sample_names.end(), name))
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
					if (parsed_sample_names.end() != it)
					{
						auto const idx1(it->second);
						cb(idx1);
					}
				}
			}
			
			if (alt_mapping.empty())
				return false;
		}
		
		// Number the new ALT values.
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
				if (gt.alt && vcf::sample_genotype::NULL_ALLELE != gt.alt)
					gt.alt = alt_mapping[gt.alt];
			}
		}
		
		return true;
	}


	bool check_zygosity(vcf::transient_variant::variant_sample_type const &sample, std::uint64_t const expected_zygosity, vcf::genotype_field_gt const &gt_field)
	{
		static_assert(0x7fff == vcf::sample_genotype::NULL_ALLELE); // Should be positive and small enough s.t. the sum can fit into std::uint64_t or similar.
		auto const &gt(gt_field(sample));
		auto const zygosity(std::accumulate(gt.begin(), gt.end(), std::uint64_t(0), [](auto const acc, vcf::sample_genotype const &sample_gt){
			return acc + sample_gt.alt;
		}));
		return zygosity == expected_zygosity;
	}


	bool check_zygosity(vcf::transient_variant const &var, std::int16_t const expected_zygosity)
	{
		auto const *gt_field(get_variant_format(var).gt);
		for (auto const &sample : var.samples())
		{
			if (check_zygosity(sample, expected_zygosity, *gt_field))
				return true;
		}

		return false;
	}


	bool check_zygosity(
		vcf::reader const &reader,
		vcf::transient_variant const &var,
		std::int16_t const expected_zygosity,
		std::vector <std::string> const &sample_names,
		bool const should_exclude_samples
	)
	{
		auto const *gt_field(get_variant_format(var).gt);
		auto const &parsed_sample_names(reader.sample_indices_by_name());
		auto const &samples(var.samples());

		if (should_exclude_samples)
		{
			for (auto const &pair : parsed_sample_names)
			{
				auto const &name(pair.first);
				// Check whether the current sample name is also in the excluded list.
				if (std::binary_search(sample_names.begin(), sample_names.end(), name))
					continue;

				auto const idx1(pair.second);
				libbio_assert_lt(0, idx1);
				auto const &sample(samples[idx1 - 1]);
				if (check_zygosity(sample, expected_zygosity, *gt_field))
					return true;
			}

			return false;
		}
		else
		{
			for (auto const &sample_name : sample_names)
			{
				auto const it(parsed_sample_names.find(sample_name));
				if (parsed_sample_names.end() == it)
					continue;

				auto const idx1(it->second);
				libbio_assert_lt(0, idx1);
				auto const &sample(samples[idx1 - 1]);
				if (check_zygosity(sample, expected_zygosity, *gt_field))
					return true;
			}

			return false;
		}
	}
	
	
	void output_header(vcf::reader const &reader, std::ostream &stream, sample_name_vector const &sample_names, bool const exclude_samples)
	{
		auto const &metadata(reader.metadata());
		
		stream << "##fileformat=VCFv4.3\n";
		metadata.visit_all_metadata([&stream](vcf::metadata_base const &meta){
			meta.output_vcf(stream);
		});
		
		if (reader.sample_indices_by_name().empty())
			stream << "#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\n";
		else
		{
			stream << "#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\tFORMAT";
			if (sample_names.empty() || exclude_samples)
			{
				std::vector <std::string const *> output_sample_names(reader.sample_count());
				std::size_t found_count{};
				for (auto const &[name, number] : reader.sample_indices_by_name())
				{
					libbio_assert_lt(0, number);

					auto const did_find(std::binary_search(sample_names.begin(), sample_names.end(), name));
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
	}
	
	
	void output_vcf(
		vcf::reader &reader,
		variant_printer_base &printer,
		std::ostream &stream,
		sample_name_vector const &sample_names,
		alt_number_map &alt_mapping,
		bool const exclude_samples,
		char const *expected_chr_id,
		std::int16_t const expected_zygosity
	)
	{
		/*
		if (!sample_names.empty())
			check_sample_names(reader, sample_names);
		*/
		
		output_header(reader, stream, sample_names, exclude_samples);
		printer.prepare(reader);
		
		bool should_continue(false);
		std::size_t lineno{};
		reader.parse_nc(
			[
				&reader,
				&printer,
				&stream,
				&sample_names,
				exclude_samples,
				&alt_mapping,
				&lineno,
				expected_chr_id,	// Pointer
				expected_zygosity	// std::int16_t
			](vcf::transient_variant &var){
				++lineno;

				// FIXME: use vcf::reader’s filtering facility (and try to output correct line number counts anyway).
				if (expected_chr_id && var.chrom_id() != expected_chr_id)
					goto continue_parsing;
				
				printer.begin_variant(reader);

				if ((!sample_names.empty()) || exclude_samples)
				{
					// FIXME: a variant may be excluded if there are no non-zero and non-missing GT values.
					if (
						modify_variant(var, alt_mapping, sample_names, exclude_samples) &&
						(-1 == expected_zygosity || check_zygosity(reader, var, expected_zygosity, sample_names, exclude_samples))
					)
					{
						vcf::output_vcf(printer, stream, var);
					}
				}
				else
				{
					if (-1 == expected_zygosity || check_zygosity(var, expected_zygosity))
						vcf::output_vcf(printer, stream, var);
				}

			continue_parsing:
				printer.end_variant(reader);
				if (0 == lineno % 1000000)
					lb::log_time(std::cerr) << "Handled " << lineno << " lines…\n";
				
				return true;
			}
		);
	}


	void output_vcf(
		vcf::reader &reader,
		variant_printer_base &variant_printer,
		lb::file_ostream &output_stream,
		sample_name_vector const &sample_names,
		alt_number_map &alt_mapping,
		gengetopt_args_info const &args_info
	)
	{
		output_vcf(
			reader,
			variant_printer,
			args_info.output_given ? output_stream : std::cout,
			sample_names,
			alt_mapping,
			(args_info.exclude_samples_given ?: sample_names.empty()),
			args_info.chromosome_arg,
			args_info.zygosity_arg
		);
	}


	template <typename t_variant_printer_base>
	void output_vcf(
		vcf::reader &reader,
		lb::file_ostream &output_stream,
		sample_name_vector const &sample_names,
		gengetopt_args_info const &args_info
	)
	{
		bool const should_exclude_samples(args_info.exclude_samples_given ?: sample_names.empty());
		alt_number_map alt_mapping;
		if (should_exclude_samples)
		{
			sample_filtering_variant_printer <t_variant_printer_base> printer(sample_names, alt_mapping, should_exclude_samples);
			output_vcf(reader, printer, output_stream, sample_names, alt_mapping, args_info);
		}
		else
		{
			variant_printer <t_variant_printer_base> printer{};
			output_vcf(reader, printer, output_stream, sample_names, alt_mapping, args_info);
		}
	}
}


int main(int argc, char **argv)
{
	gengetopt_args_info args_info;
	if (0 != cmdline_parser(argc, argv, &args_info))
		std::exit(EXIT_FAILURE);
	
	std::ios_base::sync_with_stdio(false);	// Don't use C style IO after calling cmdline_parser.
	std::cin.tie(nullptr);					// We don't require any input from the user.

	// Check the zygosity parameter.
	if (args_info.zygosity_arg < -1)
	{
		std::cerr << "ERROR: Zygosity should be either non-negative or -1 for no filtering." << std::endl;
		std::exit(EXIT_FAILURE);
	}
	
	// Open the variant file.
	// FIXME: use stream input, handle compressed input.
	vcf::mmap_input vcf_input;
	lb::file_ostream output_stream;
	
	vcf_input.handle().open(args_info.input_arg);
	if (args_info.output_given)
		lb::open_file_for_writing(args_info.output_arg, output_stream, lb::writing_open_mode::CREATE);
	
	// Fill sample_names.
	sample_name_vector sample_names;
	for (std::size_t i(0); i < args_info.sample_given; ++i)
		sample_names.emplace_back(args_info.sample_arg[i]);
	if (args_info.sample_names_given)
	{
		lb::file_istream stream;
		lb::open_file_for_reading(args_info.sample_names_arg, stream);
		std::string line;
		while (std::getline(stream, line))
			sample_names.push_back(line);
	}
	// Sort.
	std::sort(sample_names.begin(), sample_names.end());
	
	// Instantiate the parser and add the fields listed in the specification to the metadata.
	vcf::reader reader;
	vcf::add_reserved_info_keys(reader.info_fields());
	vcf::add_reserved_genotype_keys(reader.genotype_fields());
	
	// Parse.
	reader.set_variant_format(new variant_format());
	reader.set_input(vcf_input);
	reader.read_header();
	reader.set_parsed_fields(vcf::field::ALL);
	
	if (args_info.preserve_field_order_flag)
		output_vcf <order_preserving_variant_printer_base>(reader, output_stream, sample_names, args_info);
	else
		output_vcf <variant_printer_base>(reader, output_stream, sample_names, args_info);
	
	return EXIT_SUCCESS;
}

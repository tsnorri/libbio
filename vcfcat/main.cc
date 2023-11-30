/*
 * Copyright (c) 2019-2022 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <boost/format.hpp>
#include <libbio/assert.hh>
#include <libbio/cxxcompat.hh>
#include <libbio/file_handling.hh>
#include <libbio/vcf/parse_error.hh>
#include <libbio/vcf/subfield.hh>
#include <libbio/vcf/variant.hh>
#include <libbio/vcf/variant_printer.hh>
#include <libbio/vcf/vcf_reader.hh>
#include <map>
#include <numeric>
#include <numeric>
#include <range/v3/iterator/insert_iterators.hpp>
#include <range/v3/view/enumerate.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/take_while.hpp>
#include <set>
#include "cmdline.h"


namespace lb	= libbio;
namespace rsv	= ranges::views;
namespace vcf	= libbio::vcf;

typedef std::vector <std::string>								sample_name_vector;
typedef std::vector <std::size_t>								alt_number_vector;
typedef std::set <std::string, lb::compare_strings_transparent>	field_id_set;


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

	protected:
		vcf::genotype_ptr_vector	m_format;
		field_id_set const			*m_retained_info_fields{};
		field_id_set const			*m_retained_genotype_fields{};
		alt_number_vector const		*m_alt_mapping{};
		
	public:
		using base_class::output_info;
		using base_class::output_format;
		using base_class::output_sample;

		variant_printer_base(
			field_id_set const *retained_info_fields,
			field_id_set const *retained_genotype_fields,
			alt_number_vector const &alt_mapping
		):
			m_retained_info_fields(retained_info_fields),
			m_retained_genotype_fields(retained_genotype_fields),
			m_alt_mapping(&alt_mapping)
		{
		}

		virtual ~variant_printer_base() {}
		virtual void prepare(vcf::reader &reader) {}
		virtual void vcf_reader_did_update_variant_format(vcf::reader &reader);
		virtual void begin_variant(vcf::reader &reader) {}

	protected:
		void update_and_filter_variant_format(vcf::reader &reader);

	public:
		void output_alt(std::ostream &os, variant_type const &var) const override;
		void output_info(std::ostream &os, variant_type const &var) const override;
		void output_format(std::ostream &os, variant_type const &var) const override;
		void output_sample(std::ostream &os, variant_type const &var, variant_sample_type const &sample) const override;
	};
	
	
	class order_preserving_variant_printer_base : public variant_printer_base
	{
	protected:
		vcf::info_field_ptr_vector		m_info_fields;			// Non-owning.

	public:
		using variant_printer_base::variant_printer_base;
		using variant_printer_base::output_info;
		using variant_printer_base::output_format;
		using variant_printer_base::output_sample;

		void output_info(std::ostream &os, variant_type const &var) const override;
		void output_format(std::ostream &os, variant_type const &var) const override;
		void output_sample(std::ostream &os, variant_type const &var, variant_sample_type const &sample) const override;

		void vcf_reader_did_update_variant_format(vcf::reader &reader) override;
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
		bool const					m_exclude_samples{};
		
	public:
		sample_filtering_variant_printer() = default;
		
		sample_filtering_variant_printer(
			field_id_set const *retained_info_fields,
			field_id_set const *retained_genotype_fields,
			alt_number_vector const &alt_mapping,
			sample_name_vector const &sample_names,
			bool const exclude_samples
		):
			t_base(retained_info_fields, retained_genotype_fields, alt_mapping),
			m_sample_names(&sample_names),
			m_exclude_samples(exclude_samples)
		{
		}
		
		void output_samples(std::ostream &os, variant_type const &var) const override;
	};
	

	struct reader_delegate : public vcf::reader_default_delegate
	{
		variant_printer_base	*m_printer{};

		reader_delegate(variant_printer_base &printer):
			m_printer(&printer)
		{
		}

		void vcf_reader_did_update_variant_format(vcf::reader &reader) override { m_printer->vcf_reader_did_update_variant_format(reader); }
	};

	
	template <typename t_fields>
	auto filtered_fields(t_fields &&fields, field_id_set const &by_ids)
	{
		return fields
		| rsv::filter([&by_ids](auto const &field){ return by_ids.contains(field.get_metadata()->get_id()); });
	}


	void variant_format::reader_did_update_format(vcf::reader &reader)
	{	
		this->assign_field_ptr("GT", gt);
	}
	
	
	inline variant_format const &get_variant_format(vcf::transient_variant const &var)
	{
		libbio_assert(var.reader()->has_assigned_variant_format());
		return static_cast <variant_format const &>(var.get_format());
	}
	
	
	inline variant_format const &get_variant_format(vcf::variant const &var)
	{
		libbio_assert(var.reader()->has_assigned_variant_format());
		return static_cast <variant_format const &>(var.get_format());
	}


	void variant_printer_base::update_and_filter_variant_format(vcf::reader &reader)
	{
		ranges::copy(
			filtered_fields(
				reader.get_current_variant_format()
				| rsv::indirect,
				*m_retained_genotype_fields
			)
			| rsv::transform([](auto &field) -> vcf::genotype_field_base * { return &field; }),
			ranges::back_inserter(m_format)
		);
	}


	void variant_printer_base::vcf_reader_did_update_variant_format(vcf::reader &reader)
	{
		m_format.clear();
		if (m_retained_genotype_fields)
			update_and_filter_variant_format(reader);
	}


	void variant_printer_base::output_alt(std::ostream &os, variant_type const &var) const
	{
		if (m_alt_mapping->empty())
			base_class::output_alt(os, var);
		else
		{
			auto const &alts(var.alts());
			ranges::copy(
				*m_alt_mapping
				| rsv::enumerate
				| rsv::filter([](auto const &tup){ return SIZE_MAX != std::get <1>(tup); })
				| rsv::transform([&alts](auto const &tup) -> typename variant_type::string_type const & {
					return alts[std::get <0>(tup)].alt; 
				}),
				ranges::make_ostream_joiner(os, ",")
			);
		}
	}
	
	
	void variant_printer_base::output_info(std::ostream &os, variant_type const &var) const
	{
		if (m_retained_info_fields)
		{
			auto rng(filtered_fields(var.reader()->current_record_info_fields() | rsv::indirect, *m_retained_info_fields));
			if (rng.empty())
				os << '.';
			else
				output_info(os, var, rng);
		}
		else
		{
			base_class::output_info(os, var);
		}
	}


	void variant_printer_base::output_format(std::ostream &os, variant_type const &var) const
	{
		if (m_retained_genotype_fields)
		{
			if (m_format.empty())
				os << '.';
			else
				output_format(os, var, m_format | rsv::indirect);
		}
		else
		{
			base_class::output_format(os, var);
		}
	}


	void variant_printer_base::output_sample(std::ostream &os, variant_type const &var, variant_sample_type const &sample) const
	{
		if (m_retained_genotype_fields)
		{
			if (m_format.empty())
				os << '.';
			else
				output_sample(os, var, sample, m_format | rsv::indirect);
		}
		else
		{
			base_class::output_sample(os, var, sample);
		}
	}


	void order_preserving_variant_printer_base::vcf_reader_did_update_variant_format(vcf::reader &reader)
	{
		m_format.clear();
		if (m_retained_genotype_fields)
			update_and_filter_variant_format(reader);
		else
			m_format = reader.get_current_variant_format();
	}

	
	void order_preserving_variant_printer_base::output_info(std::ostream &os, variant_type const &var) const
	{
		auto rng(filtered_fields(var.reader()->current_record_info_fields() | rsv::indirect, *m_retained_info_fields));
		if (rng.empty())
			os << '.';
		else
			output_info(os, var, rng);
	}
	
	
	void order_preserving_variant_printer_base::output_format(std::ostream &os, variant_type const &var) const
	{
		if (m_format.empty())
			os << '.';
		else
			output_format(os, var, m_format | rsv::indirect);
	}


	void order_preserving_variant_printer_base::output_sample(
		std::ostream &os,
		variant_type const &var,
		variant_sample_type const &sample
	) const
	{
		if (m_format.empty())
			os << '.';
		else
			output_sample(os, var, sample, m_format | rsv::indirect);
	}
	
	
	template <typename t_base>
	void sample_filtering_variant_printer <t_base>::output_samples(std::ostream &os, variant_type const &var) const
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


	bool update_alt_mapping(alt_number_vector &alt_mapping, std::size_t const expected_val)
	{
		// Number the new ALT values.
		std::size_t idx{};
		for (auto &val : alt_mapping)
		{
			if (expected_val == val)
				val = idx++;
			else
				val = SIZE_MAX;
		}

		if (!idx)
			return false;

		return true;
	}
	
	
	bool modify_variant_for_sample_exclusion(vcf::transient_variant &var, alt_number_vector &alt_mapping, sample_name_vector const &sample_names, bool const exclude_samples)
	{
		// If no samples were specified, don’t modify the ALT values.
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
				auto const &sample(samples[idx1 - 1]);
				for (auto const &gt : (*gt_field)(sample))
				{
					if (vcf::sample_genotype::NULL_ALLELE == gt.alt || 0 == gt.alt)
						continue;
					alt_mapping[gt.alt - 1] *= 2; // All existing values are either 0 or 1; set to 2 iff. non-zero.
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
		}

		if (!update_alt_mapping(alt_mapping, 2))
			return false;
		
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
					gt.alt = 1 + alt_mapping[gt.alt - 1];
			}
		}
		
		return true;
	}


	bool check_zygosity(vcf::transient_variant::variant_sample_type const &sample, std::uint64_t const expected_zygosity, vcf::genotype_field_gt const &gt_field)
	{
		static_assert(0x7fff == vcf::sample_genotype::NULL_ALLELE); // Should be positive and small enough s.t. the sum can fit into std::uint64_t or similar.
		auto const &gt(gt_field(sample));
		auto const zygosity(std::accumulate(gt.begin(), gt.end(), std::uint64_t(0), [](auto const acc, vcf::sample_genotype const &sample_gt){
			return acc + (sample_gt.alt ? 1 : 0);
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
		alt_number_vector &alt_mapping,
		sample_name_vector const &sample_names,
		bool const exclude_samples,
		char const *expected_chr_id,
		char const *missing_id_prefix,
		enum_variants const variant_types,
		std::int16_t const expected_zygosity
	)
	{
		/*
		if (!sample_names.empty())
			check_sample_names(reader, sample_names);
		*/

		reader_delegate delegate(printer);
		reader.set_delegate(delegate);
		
		output_header(reader, stream, sample_names, exclude_samples);
		printer.prepare(reader);

		std::string var_id_buffer;
		std::size_t missing_id_idx{};
		
		bool should_continue(false);
		std::size_t lineno{};
		try
		{
			reader.parse_nc(
				[
					&reader,
					&printer,
					&stream,
					&sample_names,
					exclude_samples,
					&alt_mapping,
					&var_id_buffer,
					&missing_id_idx,
					&lineno,
					expected_chr_id,	// Pointer
					missing_id_prefix,	// Pointer
					variant_types,		// enum
					expected_zygosity	// std::int16_t
				](vcf::transient_variant &var){
					++lineno;

					// FIXME: use vcf::reader’s filtering facility (and try to output correct line number counts anyway).
					if (expected_chr_id && var.chrom_id() != expected_chr_id)
						goto continue_parsing;

					alt_mapping.clear();
					if (variants_arg_SNPs == variant_types)
					{
						if (1 != var.ref().size())
							goto continue_parsing;

						alt_mapping.resize(var.alts().size(), 1);

						// Count the SNPs and mark the other variants.
						std::size_t snp_count{};
						for (auto const &[idx, alt] : var.alts() | rsv::enumerate)
						{
							if (vcf::sv_type::NONE == alt.alt_sv_type && 1 == alt.alt.size())
								++snp_count;
							else
								alt_mapping[idx] = 0;
						}

						if (0 == snp_count)
							goto continue_parsing;
					}
					
					printer.begin_variant(reader);

					{
						auto &var_id(var.id());
						if (missing_id_prefix && (var_id.empty() || std::erase(var_id, ".")))
						{
							++missing_id_idx;
							var_id_buffer = boost::str(boost::format("%1%%2%") % missing_id_prefix % missing_id_idx);
							var_id.emplace_back(var_id_buffer);
						}
					}

					if ((!sample_names.empty()) || exclude_samples)
					{
						if (alt_mapping.empty())
						{
							alt_mapping.resize(var.alts().size());
							std::fill(alt_mapping.begin(), alt_mapping.end(), 1);
						}

						// FIXME: a variant may be excluded if there are no non-zero and non-missing GT values.
						if (
							modify_variant_for_sample_exclusion(var, alt_mapping, sample_names, exclude_samples) &&
							(-1 == expected_zygosity || check_zygosity(reader, var, expected_zygosity, sample_names, exclude_samples))
						)
						{
							vcf::output_vcf(printer, stream, var);
						}
					}
					else if (-1 == expected_zygosity || check_zygosity(var, expected_zygosity))
					{
						if (!alt_mapping.empty())
							update_alt_mapping(alt_mapping, 1);
						
						vcf::output_vcf(printer, stream, var);
					}
					
				continue_parsing:
					if (0 == lineno % 1000000)
						lb::log_time(std::cerr) << "Handled " << lineno << " lines…\n";
					
					return true;
				}
			);
		}
		catch (vcf::parse_error const &exc)
		{
			std::cerr << "ERROR: Parse error on line " << (reader.last_header_lineno() + 1 + lineno) << ": " << exc.what() << '\n';
			std::exit(EXIT_FAILURE);
		}
	}


	void output_vcf(
		vcf::reader &reader,
		variant_printer_base &variant_printer,
		lb::file_ostream &output_stream,
		alt_number_vector &alt_mapping,
		sample_name_vector const &sample_names,
		gengetopt_args_info const &args_info
	)
	{
		output_vcf(
			reader,
			variant_printer,
			args_info.output_given ? output_stream : std::cout,
			alt_mapping,
			sample_names,
			(args_info.exclude_samples_given ?: sample_names.empty()),
			args_info.chromosome_arg,
			args_info.replace_missing_id_arg,
			args_info.variants_arg,
			args_info.zygosity_arg
		);
	}


	template <typename t_variant_printer_base>
	void output_vcf(
		vcf::reader &reader,
		lb::file_ostream &output_stream,
		sample_name_vector const &sample_names,
		field_id_set const *retained_info_fields,
		field_id_set const *retained_genotype_fields,
		gengetopt_args_info const &args_info
	)
	{
		bool const should_exclude_samples(args_info.exclude_samples_given ?: sample_names.empty());
		alt_number_vector alt_mapping;
		if (should_exclude_samples)
		{
			sample_filtering_variant_printer <t_variant_printer_base> printer(
				retained_info_fields,
				retained_genotype_fields,
				alt_mapping,
				sample_names,
				should_exclude_samples
			);
			output_vcf(reader, printer, output_stream, alt_mapping, sample_names, args_info);
		}
		else
		{
			variant_printer <t_variant_printer_base> printer(retained_info_fields, retained_genotype_fields, alt_mapping);
			output_vcf(reader, printer, output_stream, alt_mapping, sample_names, args_info);
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

	// Info and genotype fields to be retained.
	field_id_set retained_info_fields;
	field_id_set retained_genotype_fields;
	{
		auto const &metadata(reader.metadata());
		auto const &info_fields(metadata.info());
		auto const &genotype_fields(metadata.format());

		for (unsigned int i(0); i < args_info.info_field_given; ++i)
		{
			auto const *field_id(args_info.info_field_arg[i]);
			if (!genotype_fields.contains(field_id))
			{
				std::cerr << "ERROR: The VCF file did not contain an INFO header for the genotype field id “" << field_id << "”." << std::endl;
				std::exit(EXIT_FAILURE);
			}

			retained_info_fields.emplace(field_id);
		}

		for (unsigned int i(0); i < args_info.genotype_field_given; ++i)
		{
			auto const *field_id(args_info.genotype_field_arg[i]);
			if (!genotype_fields.contains(field_id))
			{
				std::cerr << "ERROR: The VCF file did not contain a FORMAT header for the genotype field id “" << field_id << "”." << std::endl;
				std::exit(EXIT_FAILURE);
			}

			retained_genotype_fields.emplace(field_id);
		}
	}
	
	auto const *retained_info_fields_(args_info.omit_info_given || !retained_genotype_fields.empty() ? &retained_genotype_fields : nullptr);
	auto const *retained_genotype_fields_(retained_genotype_fields.empty() ? nullptr : &retained_genotype_fields);
	if (args_info.preserve_field_order_flag)
		output_vcf <order_preserving_variant_printer_base>(reader, output_stream, sample_names, retained_info_fields_, retained_genotype_fields_, args_info);
	else
		output_vcf <variant_printer_base>(reader, output_stream, sample_names, retained_info_fields_, retained_genotype_fields_, args_info);
	
	return EXIT_SUCCESS;
}

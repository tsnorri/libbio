/*
 * Copyright (c) 2022 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <libbio/vcf/variant.hh>
#include <libbio/vcf/vcf_reader.hh>
#include <range/v3/view/enumerate.hpp>
#include "cmdline.h"

namespace lb	= libbio;
namespace rsv	= ranges::views;
namespace vcf	= libbio::vcf;


namespace {

	class variant_validator final : public vcf::variant_validator
	{
	protected:
		std::string m_chr_id;
		std::size_t	m_prev_pos{};

	public:
		variant_validator(char const *chr_id):
			m_chr_id(chr_id)
		{
		}

		vcf::variant_validation_result validate(vcf::transient_variant const &var) override
		{
			if (m_chr_id != var.chrom_id())
				return vcf::variant_validation_result::SKIP;

			auto const pos(var.zero_based_pos());
			libbio_always_assert_lte(m_prev_pos, pos);
			m_prev_pos = pos;
			return vcf::variant_validation_result::PASS;
		}
	};


	// FIXME: move to the library part.
	template <typename... t_mixins>
	struct variant_format final : public vcf::variant_format, public t_mixins...
	{
		struct copy_mixins_tag {};

		static variant_format const &get(vcf::transient_variant const &var);
		static variant_format const &get(vcf::variant const &var);

		// Initialize each of t_mixins with one parameter from args.
		template <typename... t_args>
		variant_format(t_args const & ... args):
			t_mixins(args)...
		{
			static_assert(sizeof...(t_mixins) == sizeof...(t_args));
		}

		variant_format(variant_format const &other, copy_mixins_tag const &):
			t_mixins(other)...
		{
		}

		// Return a new empty instance of this class.
		virtual variant_format *new_instance() const override { return new variant_format(*this, copy_mixins_tag{}); }

		template <typename t_base>
		auto field() const { static_assert(std::is_base_of_v <t_base, variant_format>); return t_base::field_(); }

		virtual void reader_did_update_format(vcf::reader &reader) override;
	};


	template <typename... t_mixins>
	variant_format <t_mixins...> const &variant_format <t_mixins...>::get(vcf::transient_variant const &var)
	{
		libbio_assert(var.reader()->has_assigned_variant_format());
		return static_cast <variant_format const &>(var.get_format());
	}
	

	template <typename... t_mixins>
	variant_format <t_mixins...> const &variant_format <t_mixins...>::get(vcf::variant const &var)
	{
		libbio_assert(var.reader()->has_assigned_variant_format());
		return static_cast <variant_format const &>(var.get_format());
	}


	template <typename... t_mixins>
	void variant_format <t_mixins...>::reader_did_update_format(vcf::reader &reader)
	{	
		(this->assign_field_ptr(t_mixins::name(), t_mixins::m_field_ptr), ...);
	}
	
	
	template <typename t_field>
	class field_mixin
	{
		template <typename...>
		friend struct variant_format;

	protected:
		std::string	m_name{};
		t_field		*m_field_ptr{};

	public:
		field_mixin(char const *name):
			m_name(name)
		{
		};

		std::string const &name() const { return m_name; }
		t_field *field_() const { return m_field_ptr; }
	};

	typedef field_mixin <vcf::genotype_field_gt>	gt_field_t;
	typedef field_mixin <vcf::genotype_field_base>	ps_field_generic_t;


	void output_variant_distances(vcf::reader &reader)
	{
		std::cout << "DISTANCE\n";
		reader.set_parsed_fields(vcf::field::POS);
		bool is_first{true};
		std::size_t prev_pos{};
		reader.parse(
			[
				&reader,
				&is_first,
				&prev_pos
			](vcf::transient_variant const &var){
				if (is_first)
				{
					is_first = false;
					prev_pos = var.zero_based_pos();
					return true;
				}

				auto const pos(var.zero_based_pos());
				std::cout << (pos - prev_pos) << '\n';
				prev_pos = pos;
				return true;
			}
		);
	}


	void output_counts_per_chr_copy(vcf::reader &reader, char const *gt_field_id)
	{
		typedef variant_format <gt_field_t>	variant_format;

		reader.set_variant_format(new variant_format(gt_field_id));
		reader.set_parsed_fields(vcf::field::ALL);

		auto const &sample_names(reader.sample_names_by_index());
		std::vector <std::size_t> counts(2 * sample_names.size(), 0); // FIXME: handle non-diploid.
		reader.parse(
			[
				&reader,
				&counts
			](vcf::transient_variant const &var){
				auto const * const gt_field(variant_format::get(var).field <gt_field_t>());
				if (gt_field)
				{
					for (auto const &[idx, sample] : rsv::enumerate(var.samples()))
					{
						std::vector <vcf::sample_genotype> const &gt((*gt_field)(sample));
						for (auto const &[idx_, gt_val] : rsv::enumerate(gt))
						{
							if (gt_val.alt) // FIXME: handle more than one ALT.
								++counts[2 * idx + idx_]; // FIXME: handle non-diploid.
						}
					}
				}
				return true;
			}
		);

		for (auto const &[idx, sample_name] : rsv::enumerate(sample_names))
		{
			std::cout << sample_name ;
			for (std::size_t i(0); i < 2; ++i) // FIXME: handle non-diploid.
				std::cout << '\t' << counts[2 * idx + i];
			std::cout << '\n';
		}
	}
	
	
	void output_phase_sets(vcf::reader &reader, char const *ps_field_id)
	{
		typedef variant_format <ps_field_generic_t>	variant_format;

		std::cout << "LINENO\tPHASE_SET\n";
		reader.set_variant_format(new variant_format(ps_field_id));
		reader.set_parsed_fields(vcf::field::ALL);
		std::size_t phase_set_not_set{};
		reader.parse(
			[
				&reader,
				&phase_set_not_set
			](vcf::transient_variant const &var){
				auto const * const ps_field(variant_format::get(var).field <ps_field_generic_t>());
				if (ps_field)
				{
					for (auto const &sample : var.samples())
					{
						std::cout << var.lineno() << '\t';
						ps_field->output_vcf_value(std::cout, sample);
						std::cout << '\n';
					}
				}
				else
				{
					phase_set_not_set += var.samples().size();
				}

				return true;
			}
		);
		std::cerr << "Phase set not set: " << phase_set_not_set << '\n';
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
	vcf_input.handle().open(args_info.input_arg);
	
	// Instantiate the parser and add the fields listed in the specification to the metadata.
	vcf::reader reader;
	vcf::add_reserved_info_keys(reader.info_fields());
	vcf::add_reserved_genotype_keys(reader.genotype_fields());

	variant_validator validator(args_info.chr_arg);
	reader.set_variant_validator(validator);
	
	reader.set_input(vcf_input);
	reader.read_header();

	if (args_info.variant_distances_given)
		output_variant_distances(reader);
	else if (args_info.phase_sets_given)
		output_phase_sets(reader, args_info.ps_id_arg);
	else if (args_info.counts_per_chr_copy_given)
		output_counts_per_chr_copy(reader, args_info.gt_id_arg);
	else
	{
		std::cerr << "ERROR: No mode given." << std::endl;
		return EXIT_FAILURE;
	}
	
	return EXIT_SUCCESS;
}

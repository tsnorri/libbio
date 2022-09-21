/*
 * Copyright (c) 2019-2022 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VARIANT_PRINTER_HH
#define LIBBIO_VARIANT_PRINTER_HH

#include <libbio/vcf/variant/variant_decl.hh>
#include <ostream>
#include <range/v3/algorithm/copy.hpp>
#include <range/v3/iterator/stream_iterators.hpp>
#include <range/v3/view/indirect.hpp>
#include <range/v3/view/map.hpp>
#include <range/v3/view/remove_if.hpp>


namespace libbio::vcf {
	
	template <typename t_variant>
	class variant_printer_base
	{
	public:
		typedef t_variant						variant_type;
		typedef typename t_variant::sample_type	variant_sample_type;
		
	public:
		virtual ~variant_printer_base() {}
		
		virtual bool should_print() const { return true; }
		
		virtual inline void output_chrom(std::ostream &os, variant_type const &var) const;
		virtual inline void output_pos(std::ostream &os, variant_type const &var) const;
		virtual inline void output_id(std::ostream &os, variant_type const &var) const;
		virtual inline void output_ref(std::ostream &os, variant_type const &var) const;
		virtual inline void output_alt(std::ostream &os, variant_type const &var) const;
		virtual inline void output_qual(std::ostream &os, variant_type const &var) const;
		virtual inline void output_filter(std::ostream &os, variant_type const &var) const;
		virtual inline void output_info(std::ostream &os, variant_type const &var) const;
		virtual inline void output_format(std::ostream &os, variant_type const &var) const;
		
		virtual inline void output_sample(std::ostream &os, variant_type const &var, variant_sample_type const &sample) const;
		virtual inline void output_samples(std::ostream &os, variant_type const &var) const;
		
		// Templates so that a view may be passed as info_fields or genotype_fields.
		// Note that t_fields may not be const in this case, since e.g. filtering view’s empty() is non-const.
		template <typename t_fields>
		inline void output_info(std::ostream &os, variant_type const &var, t_fields &&info_fields) const;
		
		template <typename t_fields>
		inline void output_format(std::ostream &os, variant_type const &var, t_fields &&genotype_fields) const;
		
		template <typename t_fields>
		inline void output_sample(std::ostream &os, variant_type const &var, variant_sample_type const &sample, t_fields &&genotype_fields) const;
		
		// Convenience function.
		void output_variant(std::ostream &os, variant_type const &var) const;
	};
	
	
	template <typename t_variant>
	class variant_printer final : public variant_printer_base <t_variant>
	{
	};
	
	
	template <typename t_variant>
	void variant_printer_base <t_variant>::output_chrom(std::ostream &os, variant_type const &var) const
	{
		// CHROM
		os << var.chrom_id();
	}
	
	
	template <typename t_variant>
	void variant_printer_base <t_variant>::output_pos(std::ostream &os, variant_type const &var) const
	{
		// POS
		os << var.pos();
	}
	
	
	template <typename t_variant>
	void variant_printer_base <t_variant>::output_id(std::ostream &os, variant_type const &var) const
	{
		// ID
		ranges::copy(var.id(), ranges::make_ostream_joiner(os, ","));
	}
	
	
	template <typename t_variant>
	void variant_printer_base <t_variant>::output_ref(std::ostream &os, variant_type const &var) const
	{
		// REF
		os << var.ref();
	}
	
	
	template <typename t_variant>
	void variant_printer_base <t_variant>::output_alt(std::ostream &os, variant_type const &var) const
	{
		// ALT
		auto const &alts(var.alts());
		if (0 == alts.size())
			os << '.';
		else
		{
			ranges::copy(
				var.alts() | ranges::views::transform([](auto const &va) -> typename variant_type::string_type const & { return va.alt; }),
				ranges::make_ostream_joiner(os, ",")
			);
		}
	}
	
	
	template <typename t_variant>
	void variant_printer_base <t_variant>::output_qual(std::ostream &os, variant_type const &var) const
	{
		// QUAL
		auto const qual(var.qual());
		if (abstract_variant::UNKNOWN_QUALITY == qual)
			os << '.';
		else
			os << var.qual();
	}
	
	
	template <typename t_variant>
	void variant_printer_base <t_variant>::output_filter(std::ostream &os, variant_type const &var) const
	{
		// FILTER
		auto const &filters(var.filters());
		if (filters.empty())
			os << "PASS";
		else
		{
			ranges::copy(
				var.filters() | ranges::views::transform([](auto const *filter){ return filter->get_id(); }),
				ranges::make_ostream_joiner(os, ";")
			);
		}
	}


	template <typename t_variant>
	template <typename t_fields>
	void variant_printer_base <t_variant>::output_info(
		std::ostream &os,
		variant_type const &var,
		t_fields &&info_fields
	) const
	{
		if (info_fields.empty())
		{
			os << '.';
			return;
		}

		bool is_first(true);
		for (auto const &field : info_fields)
		{
			if (field.output_vcf_value(os, var, (is_first ? "" : ";")))
				is_first = false;
		}
	}
	
	
	template <typename t_variant>
	void variant_printer_base <t_variant>::output_info(std::ostream &os, variant_type const &var) const
	{
		// INFO
		auto const *reader(var.reader());
		libbio_always_assert(reader);
		output_info(os, var, reader->info_fields_in_headers() | ranges::views::indirect);
	}


	template <typename t_variant>
	template <typename t_fields>
	void variant_printer_base <t_variant>::output_format(std::ostream &os, variant_type const &var, t_fields &&fields) const
	{
		ranges::copy(
			fields	|	ranges::views::remove_if([](auto const &ff) -> bool {
							return (metadata_value_type::NOT_PROCESSED == ff.metadata_value_type());
						})
					|	ranges::views::transform([](auto const &ff) -> std::string const & {
							auto const *meta(ff.get_metadata());
							libbio_assert(meta);
							return meta->get_id();
						}),
			ranges::make_ostream_joiner(os, ":")
		);
	}
	
	
	template <typename t_variant>
	void variant_printer_base <t_variant>::output_format(std::ostream &os, variant_type const &var) const
	{
		// FORMAT
		auto const &fields(var.get_format().fields_by_identifier());
		output_format(os, var, fields | ranges::views::values | ranges::views::indirect);
	}
	
	
	template <typename t_variant>
	template <typename t_fields>
	void variant_printer_base <t_variant>::output_sample(
		std::ostream &os,
		variant_type const &var,
		variant_sample_type const &sample,
		t_fields &&fields
	) const
	{
		// One sample
		bool is_first(true);
		for (auto const &field : fields)
		{
			if (!is_first)
				os << ':';
			field.output_vcf_value(os, sample);
			is_first = false;
		}
	}
	
	
	template <typename t_variant>
	void variant_printer_base <t_variant>::output_sample(std::ostream &os, variant_type const &var, variant_sample_type const &sample) const
	{
		auto const &fields(var.get_format().fields_by_identifier());
		output_sample(os, var, sample, fields | ranges::views::values | ranges::views::indirect);
	}
	
	
	template <typename t_variant>
	void variant_printer_base <t_variant>::output_samples(std::ostream &os, variant_type const &var) const
	{
		// Samples
		bool is_first_sample(true);
		for (auto const &sample : var.samples())
		{
			if (!is_first_sample)
				os << '\t';
			is_first_sample = false;
			
			this->output_sample(os, var, sample);
		}
	}
	
	
	template <typename t_variant>
	void variant_printer_base <t_variant>::output_variant(std::ostream &os, variant_type const &var) const
	{
		auto const *reader(var.reader());
		if (!reader)
		{
			os << "# Empty variant\n";
			return;
		}
		
		this->output_chrom(os, var); os << '\t';
		this->output_pos(os, var); os << '\t';
		this->output_id(os, var); os << '\t';
		this->output_ref(os, var); os << '\t';
		this->output_alt(os, var); os << '\t';
		this->output_qual(os, var); os << '\t';
		this->output_filter(os, var); os << '\t';
		this->output_info(os, var); os << '\t';
		this->output_format(os, var); os << '\t';
		this->output_samples(os, var); os << '\n';
	}
	
	
	template <typename t_variant>
	void output_vcf(
		variant_printer_base <t_variant> const &printer,
		std::ostream &os,
		t_variant const &var
	)
	{
		printer.output_variant(os, var);
	}
	
	
	// Using formatted_variant is the easiest way to check that the given parameter is
	// a variant, and the extra functionality in variant and transient_variant is not needed here.
	template <template <typename> typename t_printer, typename t_string, typename t_format_access>
	void output_vcf(std::ostream &os, formatted_variant <t_string, t_format_access> const &var)
	{
		t_printer <formatted_variant <t_string, t_format_access>> printer;
		output_vcf(printer, os, var);
	}
	
	
	template <typename t_string, typename t_format_access>
	void output_vcf(std::ostream &os, formatted_variant <t_string, t_format_access> const &var)
	{
		output_vcf <variant_printer>(os, var);
	}
}

#endif

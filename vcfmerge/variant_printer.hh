/*
 * Copyright (c) 2020 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCFMERGE_VARIANT_PRINTER_HH
#define LIBBIO_VCFMERGE_VARIANT_PRINTER_HH

#include <libbio/vcf/variant_printer.hh>
#include <ostream>


namespace libbio::vcfmerge {
	
	// Base class for variant printers.
	template <typename t_variant>
	class variant_printer_base : public vcf::variant_printer_base <t_variant>
	{
	public:
		typedef typename vcf::variant_printer_base <t_variant>::variant_type variant_type;
		
	protected:
		std::size_t	m_total_samples{};
		std::size_t	m_sample_ploidy{};
		bool		m_samples_are_phased{};
		
	public:
		variant_printer_base(std::size_t const total_samples, std::size_t const sample_ploidy, bool const samples_are_phased):
			m_total_samples(total_samples),
			m_sample_ploidy(sample_ploidy),
			m_samples_are_phased(samples_are_phased)
		{
		}
		
		void set_total_samples(std::size_t const total_samples) { m_total_samples = total_samples; }
		
		void output_format(std::ostream &os, variant_type const &var) const override
		{
			os << "GT";
		}
		
		// Silence a compiler warning by making the following function not hide the inherited one.
		using vcf::variant_printer_base <t_variant>::output_sample;
		
		void output_sample(std::ostream &os, bool const is_active, char const separator, bool &is_first) const
		{
			if (is_first)
				is_first = false;
			else
				os << '\t';
			
			os << is_active;
			for (std::size_t i(1); i < m_sample_ploidy; ++i)
				os << separator << is_active;
		}
		
	protected:
		void output_sample_range(std::ostream &os, std::size_t lb, std::size_t const rb, bool &is_first) const
		{
			char const separator(m_samples_are_phased ? '|' : '/');
			while (lb < rb)
			{
				output_sample(os, false, separator, is_first);
				++lb;
			}
		}
	};
	
	
	template <typename t_variant>
	class variant_printer_gs final : public variant_printer_base <t_variant>
	{
	public:
		typedef typename variant_printer_base <t_variant>::variant_type	variant_type;
		
	protected:
		std::size_t	m_active_sample_index{};
		
	public:
		using variant_printer_base <t_variant>::variant_printer_base;
		
		void set_active_sample_index(std::size_t const idx) { m_active_sample_index = idx; }
		
		void output_samples(std::ostream &os, variant_type const &var) const override
		{
			bool is_first(true);
			char const separator(this->m_samples_are_phased ? '|' : '/');
			
			this->output_sample_range(os, 0, m_active_sample_index, is_first);
			this->output_sample(os, true, separator, is_first);
			this->output_sample_range(os, 1 + m_active_sample_index, this->m_total_samples, is_first);
		}
	};
	
	
	template <typename t_variant>
	class variant_printer_ms final : public variant_printer_base <t_variant>
	{
	public:
		typedef typename variant_printer_base <t_variant>::variant_type	variant_type;
		
	protected:
		std::size_t	m_active_sample_lb{};
		std::size_t	m_active_sample_rb{};
		
	public:
		using variant_printer_base <t_variant>::variant_printer_base;
		
		void set_active_sample_range(std::size_t const lb, std::size_t const rb) { m_active_sample_lb = lb; m_active_sample_rb = rb; }
		
		void output_samples(std::ostream &os, variant_type const &var) const override
		{
			auto const &samples(var.samples());
			libbio_assert_eq(m_active_sample_rb - m_active_sample_lb, samples.size());
			
			// Get the GT field.
			auto const &fields(var.get_format().fields_by_identifier());
			auto const it(fields.find("GT"));
			libbio_always_assert_neq_msg(it, fields.end(), "Expected variant to have a GT field");
			auto const &gt_field(*it->second);
			
			// Output the samples.
			bool is_first(true);
			this->output_sample_range(os, 0, m_active_sample_lb, is_first);
			
			for (auto const &sample : samples)
			{
				if (is_first)
					is_first = false;
				else
					os << '\t';
				
				gt_field.output_vcf_value(os, sample);
			}
			
			this->output_sample_range(os, m_active_sample_rb, this->m_total_samples, is_first);
		}
	};
}

#endif

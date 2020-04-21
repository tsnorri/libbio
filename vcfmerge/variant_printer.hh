/*
 * Copyright (c) 2020 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCFMERGE_VARIANT_PRINTER_HH
#define LIBBIO_VCFMERGE_VARIANT_PRINTER_HH

#include <libbio/vcf/variant_printer.hh>
#include <ostream>


namespace libbio::vcfmerge {
	
	template <typename t_variant>
	class variant_printer final : public vcf::variant_printer_base <t_variant>
	{
	public:
		typedef typename vcf::variant_printer_base <t_variant>::variant_type variant_type;
		
	protected:
		std::size_t	m_total_samples{};
		std::size_t	m_active_sample_index{};
		std::size_t	m_sample_ploidy{};
		bool		m_samples_are_phased{};
		
	public:
		variant_printer(std::size_t const total_samples, std::size_t const sample_ploidy, bool const samples_are_phased):
			m_total_samples(total_samples),
			m_sample_ploidy(sample_ploidy),
			m_samples_are_phased(samples_are_phased)
		{
		}
		
		void set_active_sample_index(std::size_t const idx) { m_active_sample_index = idx; }
		
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
		
		void output_samples(std::ostream &os, variant_type const &var) const override
		{
			bool is_first(true);
			char const separator(m_samples_are_phased ? '|' : '/');
			for (std::size_t i(0); i < m_active_sample_index; ++i)
				output_sample(os, false, separator, is_first);
			
			output_sample(os, true, separator, is_first);
			
			for (std::size_t i(1 + m_active_sample_index); i < m_total_samples; ++i)
				output_sample(os, false, separator, is_first);
		}
	};
}

#endif

/*
 * Copyright (c) 2020 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_MERGE_RECORDS_VARIANT_PRINTER_HH
#define LIBBIO_VCF_MERGE_RECORDS_VARIANT_PRINTER_HH

#include <libbio/assert.hh>
#include <libbio/vcf/variant_printer.hh>
#include <ostream>
#include <range/v3/all.hpp>
#include <vector>


namespace libbio::vcf_merge_records {
	
	class variant_printer final : public vcf::variant_printer_base <vcf::variant>
	{
	public:
		typedef vcf::variant				variant_type;
		typedef std::vector <std::uint16_t>	genotype_vector;
		
	protected:
		genotype_vector				*m_current_genotypes{};
		std::vector <bool> const	*m_current_phasing{};
		std::string const			*m_ref{};
		std::string const			*m_alt{};
		std::uint16_t				m_ploidy{};
		
	public:
		void set_current_genotypes(genotype_vector &genotypes) { m_current_genotypes = &genotypes; }
		void set_ref(std::string const &ref) { m_ref = &ref; }
		void set_alt(std::string const &alt) { m_alt = &alt; }
		void set_ploidy(std::uint16_t const ploidy) { m_ploidy = ploidy; }
		void set_phasing(std::vector <bool> const &phasing) { m_current_phasing = &phasing; }
		
		
		virtual inline void output_ref(std::ostream &os, variant_type const &var) const override
		{
			libbio_assert(m_ref);
			os << *m_ref;
		}
		
		
		virtual inline void output_alt(std::ostream &os, variant_type const &var) const override
		{
			libbio_assert(m_alt);
			os << *m_alt;
		}
		
		
		void output_info(std::ostream &os, variant_type const &var) const override
		{
			os << '.';
		}
		
		
		void output_format(std::ostream &os, variant_type const &var) const override
		{
			os << "GT";
		}
		
		
		void output_samples(std::ostream &os, variant_type const &var) const override
		{
			libbio_assert(m_current_genotypes);
			libbio_assert(m_current_phasing);
			libbio_assert(m_ploidy);
			libbio_assert_eq(0, m_current_genotypes->size() % m_ploidy);
			
			bool is_first_sample(true);
			std::size_t i(0);
			for (auto const &sample : *m_current_genotypes | ranges::view::chunk(m_ploidy))
			{
				if (is_first_sample)
					is_first_sample = false;
				else
					os << '\t';
				
				bool is_first_alt(true);
				for (auto const alt_idx : sample)
				{
					libbio_assert_lt(i, m_current_phasing->size());
					if (is_first_alt)
						is_first_alt = false;
					else
						os << ((*m_current_phasing)[i] ? '|' : '/');
					
					os << alt_idx;
					++i;
				}
			}
		}
	};
}

#endif

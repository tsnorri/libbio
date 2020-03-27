/*
 * Copyright (c) 2019 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <libbio/vcf/subfield.hh>


%% machine gt_parser;
%% write data;


namespace lb	= libbio;


namespace {
	
	void output_genotype(std::ostream &stream, std::vector <lb::sample_genotype> const &genotype)
	{
		bool is_first(true);
		for (auto const &gt : genotype)
		{
			if (!is_first)
				stream << (gt.is_phased ? '|' : '/');
			
			if (lb::sample_genotype::NULL_ALLELE == gt.alt)
				stream << '.';
			else
				stream << gt.alt;
			
			is_first = false;
		}
	}
}


namespace libbio {
	
	void vcf_genotype_field_gt::output_vcf_value(std::ostream &stream, variant_sample const &sample) const
	{
		output_genotype(stream, sample.m_genotype);
	}
	
	
	void vcf_genotype_field_gt::output_vcf_value(std::ostream &stream, transient_variant_sample const &sample) const
	{
		output_genotype(stream, sample.m_genotype);
	}
	
	
	bool vcf_genotype_field_gt::parse_and_assign(std::string_view const &sv, transient_variant_sample &sample, std::byte *mem) const
	{
		// Mixed phasing is possible, e.g. in VCF 4.2 specification p. 26: 0/1|2: triploid with a single phased allele.
		// (The specification says it is tetraploid but this is likely a bug.)
		sample.m_genotype.clear();
		std::uint16_t idx{};
		bool is_phased{};

		auto const *p(sv.data());
		auto const *pe(p + sv.size());
		auto const *eof(pe);
		int cs(0);
		
		%%{
			machine gt_parser;
			
			action null_allele {
				idx = sample_genotype::NULL_ALLELE;
			}
			
			action start_idx {
				idx = 0;
			}
			
			action update_idx {
				// Shift and add the current number.
				idx *= 10;
				idx += fc - '0';
			}
			
			action end_part {
				sample.m_genotype.emplace_back(idx, is_phased);
			}
			
			action is_phased {
				is_phased = true;
			}
			
			action not_phased {
				is_phased = false;
			}
			
			null_allele		= '.' >(null_allele);
			allele_idx		= (digit+) >(start_idx) $(update_idx);
			gt_part			= (null_allele | allele_idx) %(end_part);
			gt_sep			= ('|' >(is_phased)) | ('/' >(not_phased));
			gt				= gt_part (gt_sep gt_part)*;
			
			main := gt;
			
			write init;
			write exec;
		}%%
		
		return true;
	}
}

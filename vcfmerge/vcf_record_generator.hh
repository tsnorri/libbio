/*
 * Copyright (c) 2020 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCFMERGE_VCF_RECORD_GENERATOR_HH
#define LIBBIO_VCFMERGE_VCF_RECORD_GENERATOR_HH

#include <libbio/vcf/vcf_reader.hh>


namespace libbio::vcfmerge {
	
	class vcf_record_generator
	{
	protected:
		vcf::reader::parser_state	m_parser_state;
		vcf::reader					*m_reader{};
		
	public:
		vcf_record_generator() = default;
		
		vcf_record_generator(vcf::reader &reader):
			m_reader(&reader)
		{
		}
		
		bool get_next_variant(vcf::variant &out_variant)
		{
			bool retval(false);
			bool const st(m_reader->parse_one([this, &out_variant, &retval](vcf::transient_variant const &var) {
				// Not reached on EOF.
				out_variant = var;
				retval = true;
				return true;
			}, m_parser_state));
			return retval;
		}
	};
}

#endif

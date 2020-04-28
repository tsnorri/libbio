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
		bool						m_has_records{true};
		
	public:
		vcf_record_generator() = default;
		
		vcf_record_generator(vcf::reader &reader):
			m_reader(&reader)
		{
		}
		
		bool get_next_variant(vcf::variant &out_variant)
		{
			if (!m_has_records)
				return false;
			
			m_has_records = false;
			bool const st(m_reader->parse_one([this, &out_variant](vcf::transient_variant const &var) {
				// Not reached on EOF.
				out_variant = var;
				m_has_records = true;
				return true;
			}, m_parser_state));
			return m_has_records;
		}
	};
}

#endif

/*
 * Copyright (c) 2019 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_READER_SUPPORT_HH
#define LIBBIO_VCF_READER_SUPPORT_HH

#include <libbio/vcf/vcf_subfield_decl.hh>


namespace libbio {
	
	class vcf_reader_support final
	{
	private:
		vcf_info_field_placeholder		m_ifp;
		vcf_genotype_field_placeholder	m_gfp;
		
	public:
		static vcf_reader_support &get_instance()
		{
			static vcf_reader_support rs;
			return rs;
		}
		
		vcf_reader_support(vcf_reader_support const &) = delete;
		vcf_reader_support &operator=(vcf_reader_support const &) = delete;
		
		vcf_info_field_placeholder &get_info_field_placeholder() { return m_ifp; }
		vcf_genotype_field_placeholder &get_genotype_field_placeholder() { return m_gfp; }
		
	private:
		vcf_reader_support() {}
	};
}

#endif

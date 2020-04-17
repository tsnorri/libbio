/*
 * Copyright (c) 2019 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_READER_SUPPORT_HH
#define LIBBIO_VCF_READER_SUPPORT_HH

#include <libbio/vcf/subfield/placeholder.hh>


namespace libbio::vcf {
	
	class reader_support final
	{
	private:
		info_field_placeholder		m_ifp;
		genotype_field_placeholder	m_gfp;
		
	public:
		static reader_support &get_instance()
		{
			static reader_support rs;
			return rs;
		}
		
		reader_support(reader_support const &) = delete;
		reader_support &operator=(reader_support const &) = delete;
		
		info_field_placeholder &get_info_field_placeholder() { return m_ifp; }
		genotype_field_placeholder &get_genotype_field_placeholder() { return m_gfp; }
		
	private:
		reader_support() {}
	};
}

#endif

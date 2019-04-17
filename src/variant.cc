/*
 * Copyright (c) 2019 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <libbio/vcf/variant.hh>


namespace lb	= libbio;


namespace libbio {

	void variant_base::finish_copy(variant_base const &other, variant_format const &variant_format, bool const should_initialize)
	{
		if (m_info.size() || m_samples.size())
		{
			libbio_always_assert(m_reader);
	
			// Zeroed when copying.
			if (should_initialize)
				m_reader->initialize_variant(*this, variant_format.fields_by_identifier());
			m_reader->copy_variant(other, *this, variant_format.fields_by_identifier());
		}
	}
}

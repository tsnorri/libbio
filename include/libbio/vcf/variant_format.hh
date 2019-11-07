/*
 * Copyright (c) 2019 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VARIANT_FORMAT_HH
#define LIBBIO_VARIANT_FORMAT_HH

#include <libbio/assert.hh>
#include <libbio/vcf/vcf_subfield_decl.hh>


namespace libbio {
	
	class vcf_reader;
	
	
	class variant_format
	{
		friend class vcf_reader;
		
	protected:
		vcf_genotype_field_map			m_fields_by_identifier;
	
	public:
		virtual ~variant_format() {}
		virtual void reader_will_update_format(vcf_reader &reader) {}
		virtual void reader_did_update_format(vcf_reader &reader) {}
		vcf_genotype_field_map const &fields_by_identifier() const { return m_fields_by_identifier; }
		
		// Return a new empty instance of this class.
		virtual variant_format *new_instance() const { return new variant_format(); }
		
	protected:
		template <typename t_str, typename t_dst>
		inline void assign_field_ptr(t_str const &id, t_dst &dst);
	};
	
	
	typedef std::shared_ptr <variant_format>	variant_format_ptr;
	
	
	template <typename t_str, typename t_dst>
	void variant_format::assign_field_ptr(t_str const &id, t_dst &dst)
	{
		auto const it(m_fields_by_identifier.find(id));
		libbio_always_assert_neq(it, m_fields_by_identifier.end());
		dst = dynamic_cast <t_dst>(it->second.get());
	}
}

#endif

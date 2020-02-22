/*
 * Copyright (c) 2017-2019 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VARIANT_DEF_HH
#define LIBBIO_VARIANT_DEF_HH

#include <libbio/assert.hh>
#include <libbio/vcf/variant_decl.hh>
#include <libbio/vcf/vcf_reader.hh>
#include <ostream>
#include <range/v3/algorithm/copy.hpp>
#include <range/v3/iterator/stream_iterators.hpp>
#include <range/v3/view/remove_if.hpp>
#include <range/v3/view/transform.hpp>



namespace libbio {

	template <typename t_lhs_string, typename t_rhs_string>
	bool operator==(variant_alt <t_lhs_string> const &lhs, variant_alt <t_rhs_string> const &rhs)
	{
		return lhs.alt_sv_type == rhs.alt_sv_type && lhs.alt == rhs.alt;
	}

	template <typename t_lhs_string, typename t_rhs_string>
	bool operator<(variant_alt <t_lhs_string> const &lhs, variant_alt <t_rhs_string> const &rhs)
	{
		return to_underlying(lhs.alt_sv_type) < to_underlying(rhs.alt_sv_type) && lhs.alt < rhs.alt;
	}
	
	
	variant_base::variant_base(vcf_reader &reader, std::size_t const sample_count, std::size_t const info_size, std::size_t const info_alignment):
		m_reader(&reader),
		m_info(info_size, info_alignment),
		m_samples(sample_count),
		m_assigned_info_fields(reader.metadata().info().size(), false)
	{
	}
	
	
	std::size_t variant_base::zero_based_pos() const
	{
		libbio_always_assert_msg(0 != m_pos, "Unexpected position");
		return m_pos - 1;
	}


	void variant_base::reset()
	{
		m_assigned_info_fields.assign(m_assigned_info_fields.size(), false);
		m_filters.clear();
	}
	
	
	variant_format_ptr const &transient_variant_format_access::get_format_ptr(vcf_reader const &reader) const
	{
		return reader.get_variant_format_ptr();
	}
	
	
	variant_format_access::variant_format_access(vcf_reader const &reader):
		m_format(reader.get_variant_format_ptr())
	{
	}
	
	
	// Constructor.
	template <typename t_format_access>
	formatted_variant_base <t_format_access>::formatted_variant_base(
		vcf_reader &reader,
		std::size_t const sample_count,
		std::size_t const info_size,
		std::size_t const info_alignment
	):
		variant_base(reader, sample_count, info_size, info_alignment),
		t_format_access(reader)
	{
		reader.initialize_variant(*this, this->get_format().fields_by_identifier());
	}
	
	
	// Copy constructors.
	template <typename t_format_access>
	formatted_variant_base <t_format_access>::formatted_variant_base(formatted_variant_base const &other):
		variant_base(other),
		t_format_access(other)
	{
		this->finish_copy(other, get_format(), true);
	}
	
	template <typename t_format_access>
	template <typename t_other_format_access>
	formatted_variant_base <t_format_access>::formatted_variant_base(formatted_variant_base <t_other_format_access> const &other):
		variant_base(other),
		t_format_access(*this->m_reader, other)
	{
		this->finish_copy(other, get_format(), true);
	}
	
	
	// Copy assignment operator.
	template <typename t_format_access>
	auto formatted_variant_base <t_format_access>::operator=(formatted_variant_base const &other) & -> formatted_variant_base &
	{
		if (this != &other)
		{
			variant_base::operator=(other);
			t_format_access::operator=(other);
			this->finish_copy(other, get_format(), false);
		}
		return *this;
	}
	
	
	// Destructor.
	template <typename t_format_access>
	formatted_variant_base <t_format_access>::~formatted_variant_base()
	{
		if (this->m_info.size() || this->m_samples.size())
		{
			libbio_always_assert(this->m_reader);
			auto const &fields(this->get_format().fields_by_identifier());
			this->m_reader->deinitialize_variant(*this, fields);
		}
	}
	
	
	template <typename t_string, typename t_format_access>
	void variant_tpl <t_string, t_format_access>::set_id(std::string_view const &id, std::size_t const pos)
	{
		if (! (pos < m_id.size()))
			m_id.resize(pos + 1);
		
		m_id[pos] = id;
	}
	
	
	template <typename t_string, typename t_format_access>
	void variant_tpl <t_string, t_format_access>::set_alt(std::string_view const &alt, std::size_t const pos)
	{
		if (! (pos < m_alts.size()))
			m_alts.resize(pos + 1);
		
		m_alts[pos] = alt;
	}
	
	
	void transient_variant::reset()
	{
		variant_tpl <std::string_view, transient_variant_format_access>::reset();
		
		// Try to avoid deallocating the samples.
		m_id.clear();
		m_alts.clear();
	}
}

#endif

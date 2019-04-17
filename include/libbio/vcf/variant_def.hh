/*
 * Copyright (c) 2017-2019 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VARIANT_DEF_HH
#define LIBBIO_VARIANT_DEF_HH

#include <boost/function_output_iterator.hpp>
#include <boost/range/iterator_range_core.hpp>
#include <libbio/assert.hh>
#include <libbio/vcf/variant_decl.hh>
#include <libbio/vcf/vcf_reader.hh>
#include <ostream>
#include <range/v3/algorithm/copy.hpp>
#include <range/v3/utility/iterator.hpp>
#include <range/v3/view/remove_if.hpp>
#include <range/v3/view/transform.hpp>



namespace libbio {
	
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
	
	
	// Copy constructor.
	template <typename t_format_access>
	formatted_variant_base <t_format_access>::formatted_variant_base(formatted_variant_base const &other):
		variant_base(other)
	{
		if (this->m_info.size() || this->m_samples.size())
		{
			libbio_always_assert(this->m_reader);
			
			// Zeroed when copying.
			auto const &variant_format(this->get_variant_format());
			this->m_reader->initialize_variant(*this, variant_format);
			this->m_reader->copy_variant(*this, other, variant_format);
		}
	}
	
	// Copy assignment operator.
	template <typename t_format_access>
	auto formatted_variant_base <t_format_access>::operator=(formatted_variant_base const &other) & -> formatted_variant_base &
	{
		if (this->m_info.size() || this->m_samples.size())
		{
			libbio_always_assert(this->m_reader);
			auto const &variant_format(this->get_variant_format());
			this->m_reader->copy_variant(*this, other, variant_format);
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
	
	
	template <typename t_string, typename t_format_access>
	void output_vcf(std::ostream &stream, variant_tpl <t_string, t_format_access> const &var)
	{
		auto const *reader(var.reader());
		if (!reader)
		{
			stream << "# Empty variant\n";
			return;
		}
		
		auto const &fields(var.get_format().fields_by_identifier());
		
		// CHROM, POS
		stream << var.chrom_id() << '\t' << var.pos();
		
		// ID
		stream << '\t';
		ranges::copy(var.id(), ranges::make_ostream_joiner(stream, ","));
		
		// REF
		stream << '\t' << var.ref();
		
		// ALT
		stream << '\t';
		ranges::copy(
			var.alts() | ranges::view::transform([](auto const &va) -> std::string_view const & { return va.alt; }),
			ranges::make_ostream_joiner(stream, ",")
		);
		
		// QUAL
		stream << '\t' << var.qual();
			
		// FILTER
		// FIXME: output the actual value once we have it.
		stream << "\tPASS";
		
		// INFO
		{
			stream << '\t';
			bool is_first(true);
			for (auto const *field_ptr : reader->info_fields_in_headers())
			{
				if (field_ptr->output_vcf_value(stream, var, (is_first ? "" : ";")))
					is_first = false;
			}
		}
		
		// FORMAT
		stream << '\t';
		ranges::copy(
			fields	|	ranges::view::remove_if([](auto const &kv) -> bool {
							return (vcf_metadata_value_type::NOT_PROCESSED == kv.second->value_type());
						})
					|	ranges::view::transform([](auto const &kv) -> std::string const & {
							auto const *meta(kv.second->get_metadata());
							libbio_assert(meta);
							return meta->get_id(); 
						}),
			ranges::make_ostream_joiner(stream, ":")
		);
		
		// Samples
		for (auto const &sample : var.samples())
		{
			stream << '\t';
			bool is_first(true);
			for (auto const &kv : fields)
			{
				if (!is_first)
					stream << ':';
				kv.second->output_vcf_value(stream, sample);
				is_first = false;
			}
		}
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

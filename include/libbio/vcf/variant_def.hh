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
	
	variant_data::variant_data(vcf_reader &reader, std::size_t const sample_count, std::size_t const info_size, std::size_t const info_alignment):
		m_reader(&reader),
		m_info(info_size, info_alignment),
		m_samples(sample_count),
		m_assigned_info_fields(reader.metadata().info().size(), false)
	{
	}
	
	
	std::size_t variant_data::zero_based_pos() const
	{
		libbio_always_assert_msg(0 != m_pos, "Unexpected position");
		return m_pos - 1;
	}
	
	
	variant_base::variant_base(
		vcf_reader &reader,
		std::size_t const sample_count,
		std::size_t const info_size,
		std::size_t const info_alignment
	):
		variant_data(reader, sample_count, info_size, info_alignment)
	{
		this->m_reader->initialize_variant(*this);
	}
	
	// Copy constructor.
	variant_base::variant_base(variant_base const &other):
		variant_data(other)
	{
		if (this->m_info.size() || this->m_samples.size())
		{
			libbio_always_assert(this->m_reader);
			
			// Zeroed when copying.
			this->m_reader->initialize_variant(*this);
			this->m_reader->copy_variant(other, *this);
		}
	}
	
	
	variant_base &variant_base::operator=(variant_base &other) &
	{
		if (this->m_info.size() || this->m_samples.size())
		{
			libbio_always_assert(this->m_reader);
			this->m_reader->copy_variant(other, *this);
		}
		return *this;
	}
	
	
	variant_base::~variant_base()
	{
		if (this->m_info.size() || this->m_samples.size())
		{
			libbio_always_assert(this->m_reader);
			this->m_reader->deinitialize_variant(*this);
		}
	}
	
	
	template <typename t_string>
	void variant_tpl <t_string>::set_id(std::string_view const &id, std::size_t const pos)
	{
		if (! (pos < m_id.size()))
			m_id.resize(pos + 1);
		
		m_id[pos] = id;
	}
	
	
	template <typename t_string>
	void variant_tpl <t_string>::set_alt(std::string_view const &alt, std::size_t const pos)
	{
		if (! (pos < m_alts.size()))
			m_alts.resize(pos + 1);
		
		m_alts[pos] = alt;
	}
	
	
	template <typename t_string>
	void output_vcf(std::ostream &stream, variant_tpl <t_string> const &var, vcf_format_ptr_vector const &format)
	{
		auto const *reader(var.reader());
		if (!reader)
		{
			stream << "# Empty variant\n";
			return;
		}
		
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
			for (auto const &[key, info] : reader->metadata().info())
			{
				if (info.output_vcf_field(stream, var, (is_first ? "" : ";")) && is_first)
					is_first = false;
			}
		}
		
		// FORMAT
		stream << '\t';
		ranges::copy(
			format	|	ranges::view::remove_if([](auto const &format_ptr) -> bool {
							return (vcf_metadata_value_type::NOT_PROCESSED == format_ptr->get_field().value_type());
						})
					|	ranges::view::transform([](auto const &format_ptr) -> std::string const & { return format_ptr->get_id(); }),
			ranges::make_ostream_joiner(stream, ":")
		);
		
		// Samples
		for (auto const &sample : var.samples())
		{
			stream << '\t';
			bool is_first(true);
			for (auto const &format_ptr : format)
			{
				if (format_ptr->output_vcf_field(stream, sample, (is_first ? "" : ":")) && is_first)
					is_first = false;
			}
		}
	}
	
	
	void transient_variant::reset()
	{
		variant_tpl <std::string_view>::reset();
		
		// Try to avoid deallocating the samples.
		m_id.clear();
		m_alts.clear();
	}
}

#endif

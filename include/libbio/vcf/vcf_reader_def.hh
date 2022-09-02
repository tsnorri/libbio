/*
 * Copyright (c) 2019-2022 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_READER_DEF_HH
#define LIBBIO_VCF_READER_DEF_HH

#include <libbio/vcf/vcf_reader_decl.hh>
#include <libbio/vcf/subfield/generic_field.hh>


namespace libbio::vcf {
	
	reader::reader(input_base &input):
		m_input(&input)
	{
		input.reader_will_take_input();
	}
	
	void reader::set_input(input_base &input)
	{
		input.reader_will_take_input();
		m_input = &input;
	}
	
	void reader::set_parsed_fields(field max_field) {
		m_max_parsed_field = (
			m_has_samples
			? max_field
			: static_cast <field>(std::min(
				to_underlying(max_field),
				to_underlying(field::INFO)
			))
		);
	}
	
	template <typename t_key, typename t_dst>
	void reader::get_info_field_ptr(t_key const &key, t_dst &dst) const
	{
		auto const it(m_info_fields.find(key));
		libbio_always_assert_neq(it, m_info_fields.end());
		dst = dynamic_cast <std::remove_reference_t <decltype(dst)>>(it->second.get());
	}
	
	template <typename t_key, typename t_dst>
	void reader::get_genotype_field_ptr(t_key const &key, t_dst &dst) const
	{
		auto const it(m_genotype_fields.find(key));
		libbio_always_assert_neq(it, m_genotype_fields.end());
		dst = dynamic_cast <std::remove_reference_t <decltype(dst)>>(it->second.get());
	}
	
	info_field_end *reader::get_end_field_ptr() const
	{
		info_field_end *retval{};
		get_info_field_ptr("END", retval);
		return retval;
	}
	
	std::string_view reader::buffer_tail() const
	{
		// Return a view that contains the buffer contents from as close to the beginning of
		// the current line as possible.
		return std::string_view(m_current_line_or_buffer_start, m_fsm.p - m_current_line_or_buffer_start);
	}
	
	variant reader::make_empty_variant()
	{
		// See vcf_reader_header_parser.rl for m_current_variant.
		return variant(*this, sample_count(), m_current_variant.m_info.size(), m_current_variant.m_info.alignment());
	}
	
}

#endif

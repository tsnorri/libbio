/*
 * Copyright (c) 2020 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_VARIANT_VARIANT_TPL_HH
#define LIBBIO_VCF_VARIANT_VARIANT_TPL_HH

#include <libbio/cxxcompat.hh>
#include <libbio/vcf/variant/abstract_variant_decl.hh>
#include <libbio/vcf/variant/alt.hh>
#include <libbio/vcf/variant/sample.hh>
#include <string>
#include <vector>


namespace libbio::vcf {
	
	// Store the string-type variant fields as t_string.
	// This class is separated from formatted_variant mainly because maintaining
	// constructors etc. with all these variables would be cumbersome.
	template <typename t_string>
	class variant_tpl : public abstract_variant
	{
		friend class reader;
		
		template <typename>
		friend class variant_tpl;
		
		template <typename, typename>
		friend class formatted_variant;
		
	public:
		typedef t_string						string_type;
		typedef variant_alt <string_type>		variant_alt_type;
		typedef variant_sample_tpl <t_string>	variant_sample_type;
		
	protected:
		string_type								m_chrom_id{};
		string_type								m_ref{};
		std::vector <string_type>				m_id{};
		std::vector <variant_alt_type>			m_alts{};
		std::vector <variant_sample_type>		m_samples{};
		
	protected:
		template <typename t_other_sample_type>
		void copy_samples(std::vector <t_other_sample_type> const &src);
		
	public:
		variant_tpl() = default;
		
		variant_tpl(
			class reader &reader,
			std::size_t const sample_count,
			std::size_t const info_size,
			std::size_t const info_alignment
		):
			abstract_variant(reader, info_size, info_alignment),
			m_samples(sample_count)
		{
		}
		
		variant_tpl(variant_tpl const &) = default;
		variant_tpl(variant_tpl &&) = default;
		variant_tpl &operator=(variant_tpl const &) & = default;
		variant_tpl &operator=(variant_tpl &&) & = default;
		
		// Allow copying from from another specialization of variant_tpl.
		template <typename t_other_string>
		variant_tpl(variant_tpl <t_other_string> const &other):
			abstract_variant(other),
			m_chrom_id(other.m_chrom_id),
			m_ref(other.m_ref),
			m_id(other.m_id.cbegin(), other.m_id.cend()),
			m_alts(other.m_alts.cbegin(), other.m_alts.cend()),
			m_samples(other.m_samples.size())
		{
			copy_samples(other.m_samples);
		}
		
		string_type const &chrom_id() const								{ return m_chrom_id; }
		string_type const &ref() const									{ return m_ref; }
		std::vector <string_type> const &id() const						{ return m_id; }
		std::vector <variant_alt_type> &alts()							{ return m_alts; }
		std::vector <variant_alt_type> const &alts() const				{ return m_alts; }
		std::vector <variant_sample_type> &samples()					{ return m_samples; }
		std::vector <variant_sample_type> const &samples() const		{ return m_samples; }
		
		void set_chrom_id(std::string_view const &chrom_id) { m_chrom_id = chrom_id; }
		void set_ref(std::string_view const &ref) { m_ref = ref; }
		void set_id(std::string_view const &id, std::size_t const pos);
		void set_alt(std::string_view const &alt, std::size_t const pos);
	};
	
	
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
	template <typename t_other_sample_type>
	void variant_tpl <t_string>::copy_samples(std::vector <t_other_sample_type> const &src)
	{
		auto const size(src.size());
		m_samples.resize(size);
		for (std::size_t i(0); i < size; ++i)
			m_samples[i] = src[i];
	}
}

#endif

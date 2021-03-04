/*
 * Copyright (c) 2017-2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VARIANT_HH
#define LIBBIO_VARIANT_HH

#include <boost/iterator/function_output_iterator.hpp>
#include <boost/range/iterator_range_core.hpp>
#include <libbio/assert.hh>
#include <libbio/cxxcompat.hh>
#include <libbio/infix_ostream_fn.hh>
#include <libbio/types.hh>
#include <ostream>
#include <range/v3/all.hpp>
#include <vector>


namespace libbio {
	template <typename> class variant_tpl;
}


namespace libbio { namespace detail {

	template <typename t_string, typename t_other_string>
	struct variant_tpl_cmp
	{
		constexpr bool operator()(variant_tpl <t_string> const &, variant_tpl <t_other_string> const &) const { return false; }
	};
	
	template <typename t_string>
	struct variant_tpl_cmp <t_string, t_string>
	{
		bool operator()(variant_tpl <t_string> const &lhs, variant_tpl <t_string> const &rhs) const { return &lhs == &rhs; }
	};
}}


namespace libbio {
	
	class vcf_reader;
	class variant_base;
	
	
	struct genotype_field
	{
		std::uint8_t	alt{0};
		bool			is_phased{false};
	};
	
	
	class sample_field
	{
		friend class variant_base;
		
	public:
		typedef std::vector <genotype_field> genotype_vector;
		
	protected:
		genotype_vector	m_genotype;
		std::uint8_t	m_gt_count{0};
		
	public:
		
		std::uint8_t ploidy() const { return m_gt_count; }
		boost::iterator_range <genotype_vector::const_iterator> get_genotype_range() const;
		genotype_field const &get_genotype(std::uint8_t chr_idx) const { libbio_assert(chr_idx < m_gt_count); return m_genotype[chr_idx]; }
	};
	
	
	class variant_base
	{
		friend class vcf_reader;
		
	protected:
		std::vector <sample_field>	m_samples;
		std::vector <sv_type>		m_alt_sv_types;
		std::size_t					m_variant_index{0};
		std::size_t					m_sample_count{0};
		std::size_t					m_pos{0};
		std::size_t					m_qual{SIZE_MAX};
		std::size_t					m_lineno{0};
		
	public:
		variant_base(std::size_t sample_count):
			m_samples(1 + sample_count)
		{
		}
		
		explicit variant_base(variant_base const &) = default;
		variant_base &operator=(variant_base const &) & = default;
		variant_base &operator=(variant_base &&) & = default;
		
		virtual ~variant_base() {}
		
		void set_variant_index(std::size_t const idx) { m_variant_index = idx; }
		void set_lineno(std::size_t const lineno) { m_lineno = lineno; }
		void set_pos(std::size_t const pos) { m_pos = pos; }
		void set_qual(std::size_t const qual) { m_qual = qual; }
		void set_gt(std::size_t const alt, std::size_t const sample, std::size_t const idx, bool const is_phased);
		void set_alt_sv_type(sv_type const svt, std::size_t const pos);
		void reset() { m_sample_count = 0; m_alt_sv_types.clear(); }	// Try to prevent unneeded deallocation of samples.
		
		std::size_t variant_index() const								{ return m_variant_index; }
		std::size_t sample_count() const								{ return m_sample_count; }
		std::size_t lineno() const										{ return m_lineno; }
		std::size_t pos() const											{ return m_pos; }
		std::size_t zero_based_pos() const;
		std::size_t qual() const										{ return m_qual; }
		sv_type alt_sv_type(std::uint8_t const alt_idx)					{ return m_alt_sv_types[alt_idx]; }
		std::vector <sv_type> const &alt_sv_types() const				{ return m_alt_sv_types; }
		sample_field const &sample(std::size_t const sample_idx) const	{ libbio_always_assert(sample_idx <= m_sample_count); return m_samples.at(sample_idx); }
		std::vector <sample_field> const &samples() const { return m_samples; }
	};
	
	
	template <typename t_string>
	class variant_tpl : public variant_base
	{
		friend class vcf_reader;
		
		template <typename>
		friend class variant_tpl;
		
	protected:
		std::vector <t_string>			m_alts;
		std::vector <t_string>			m_id;
		t_string						m_chrom_id;
		t_string						m_ref;
		
	protected:
		template <typename t_other_string>
		void copy_vectors(variant_tpl <t_other_string> const &other);
		
	public:
		explicit variant_tpl(std::size_t sample_count = 0):
			variant_base(sample_count)
		{
		}

		template <typename t_other_string>
		explicit variant_tpl(variant_tpl <t_other_string> const &other):
			variant_base(other),
			m_chrom_id(other.m_chrom_id),
			m_ref(other.m_ref)
		{
			copy_vectors(other);
		}
		
		explicit variant_tpl(variant_tpl const &) = default;
		
		virtual ~variant_tpl() {}
		
		template <typename t_other_string>
		bool operator==(variant_tpl <t_other_string> const &other) const;
		
		template <typename t_other_string>
		bool operator!=(variant_tpl <t_other_string> const &other) const { return !(*this == other); }
		
		template <typename t_other_string>
		variant_tpl &operator=(variant_tpl <t_other_string> const &other);
		
		std::vector <t_string> const &alts() const	{ return m_alts; }
		std::vector <t_string> const &ids() const	{ return m_id; }
		t_string const &chrom_id() const			{ return m_chrom_id; }
		t_string const &ref() const					{ return m_ref; }
		std::uint8_t const alt_count() const		{ return m_alts.size(); }
		
		void reset() { variant_base::reset(); m_alts.clear(); m_id.clear(); };
		void set_chrom_id(std::string_view const &chrom_id) { m_chrom_id = chrom_id; }
		void set_ref(std::string_view const &ref) { m_ref = ref; }
		void set_id(std::string_view const &id, std::size_t const pos);
		void set_alt(std::string_view const &alt, std::size_t const pos, bool const is_complex);
		
		void output_vcf_record(std::ostream &os) const;
	};
	
	
	std::ostream &operator<<(std::ostream &os, sample_field const &sample_field);
	
	
	template <typename t_string>
	std::ostream &operator<<(std::ostream &os, variant_tpl <t_string> const &var)
	{
		// Lineno, CHROM and POS
		os << var.lineno() << ':' << var.chrom_id() << '\t' << var.pos() << '\t';
		
		// ID
		auto const &ids(var.ids());
		std::copy(
			ids.begin(),
			ids.end(),
			boost::make_function_output_iterator(infix_ostream_fn <t_string>(os, ','))
		);
		
		// REF
		os << '\t' << var.ref() << '\t';
		
		// ALT
		auto const &alts(var.alts());
		std::copy(
			alts.begin(),
			alts.end(),
			boost::make_function_output_iterator(infix_ostream_fn <t_string>(os, ','))
		);

		// QUAL
		os << '\t' << var.qual();
		
		// Samples
		for (auto const &sample : var.samples())
			os << '\t' << sample;
		
		return os;
	}
	
	
	template <typename t_string>
	void variant_tpl <t_string>::output_vcf_record(std::ostream &os) const
	{
		// #CHROM, POS
		os
			<< m_chrom_id << '\t'
			<< pos() << '\t';
		
		// ID
		std::copy(
			m_id.begin(),
			m_id.end(),
			boost::make_function_output_iterator(infix_ostream_fn <t_string>(os, ','))
		);
		os << '\t';
		
		// REF
		os << m_ref << '\t';
		
		// ALT
		std::copy(
			m_alts.begin(),
			m_alts.end(),
			boost::make_function_output_iterator(infix_ostream_fn <t_string>(os, ','))
		);
		os << '\t';
		
		// QUAL
		if (SIZE_MAX == m_qual)
			os << ".\t";
		else
			os << m_qual << '\t';
		
		// FILTER
		// FIXME: store the value.
		os << "PASS\t";
		
		// FORMAT
		// FIXME: store the format.
		os << "GT";
		
		// Samples.
		// FIXME: other fields in addition to GT.
		for (auto const &sample : m_samples)
		{
			os << '\t';
			auto const &gt_range(sample.get_genotype_range());
			os << gt_range.front().alt;
			for (auto const &gt_field : gt_range | ranges::view::drop(1))
				os << (gt_field.is_phased ? '|' : '/') << gt_field.alt;
		}
		os << '\n';
	}
	
	
	// Transient in the sense that strings point to the working
	// buffer of the reader.
	class transient_variant final : public variant_tpl <std::string_view>
	{
		friend class vcf_reader;
		
	protected:
		typedef variant_tpl superclass;
		
	public:
		using variant_tpl::variant_tpl;
		void reset();
	};
	
	
	class variant final : public variant_tpl <std::string>
	{
		friend class vcf_reader;

	protected:
		typedef variant_tpl superclass;

	public:
		using variant_tpl::variant_tpl;
		void reset();
		
		variant &operator=(transient_variant const &other) { variant_tpl::operator=(other); return *this; }
	};
	
	
	inline auto sample_field::get_genotype_range() const -> boost::iterator_range <genotype_vector::const_iterator>
	{
		auto const begin(m_genotype.cbegin());
		return boost::make_iterator_range(begin, begin + m_gt_count);
	}
	
	
	template <typename t_string>
	template <typename t_other_string>
	void variant_tpl <t_string>::copy_vectors(variant_tpl <t_other_string> const &other)
	{
		m_alts.clear();
		m_id.clear();
		m_alts.reserve(other.m_alts.size());
		m_id.reserve(other.m_id.size());
		
		for (auto const &alt : other.m_alts)
			m_alts.emplace_back(alt);
		
		for (auto const &id : other.m_id)
			m_id.emplace_back(id);
	}
	
	
	template <typename t_string>
	template <typename t_other_string>
	bool variant_tpl <t_string>::operator==(variant_tpl <t_other_string> const &other) const
	{
		detail::variant_tpl_cmp <t_string, t_other_string> cmp;
		return cmp(*this, other);
	}
	
	
	template <typename t_string>
	template <typename t_other_string>
	auto variant_tpl <t_string>::operator=(variant_tpl <t_other_string> const &other) -> variant_tpl &
	{
		if (*this != other)
		{
			variant_base::operator=(other);
			m_chrom_id.assign(other.m_chrom_id.cbegin(), other.m_chrom_id.cend());
			m_ref.assign(other.m_ref.cbegin(), other.m_ref.cend());
			copy_vectors(other);
		}
		return *this;
	}
	
	
	template <typename t_string>
	void variant_tpl <t_string>::set_id(std::string_view const &id, std::size_t const pos)
	{
		if (! (pos < m_id.size()))
			m_id.resize(pos + 1);
		
		m_id[pos] = id;
	}
	
	
	template <typename t_string>
	void variant_tpl <t_string>::set_alt(std::string_view const &alt, std::size_t const pos, bool const is_complex)
	{
		libbio_always_assert_msg(!is_complex, "Only simple ALTs are handled");
		
		if (! (pos < m_alts.size()))
			m_alts.resize(pos + 1);
		
		m_alts[pos] = alt;
	}
}

#endif

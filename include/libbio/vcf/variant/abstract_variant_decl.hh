/*
 * Copyright (c) 2020 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_VARIANT_ABSTRACT_VARIANT_DECL_HH
#define LIBBIO_VCF_VARIANT_ABSTRACT_VARIANT_DECL_HH

#include <libbio/buffer.hh>
#include <libbio/types.hh>
#include <vector>


namespace libbio::vcf {
	
	class variant_format;
	class reader;
	class metadata_filter;
	
	// Declare the constructors here s.t. the initialization code may be written more easily.
	class abstract_variant
	{
		template <typename, typename>
		friend class formatted_variant;
		
		friend class transient_variant_format_access;
		friend class variant_format_access;
		
		template <std::int32_t, metadata_value_type>
		friend class generic_info_field_base;
		
		friend class info_field_base;
		friend class reader;
		friend class storable_info_field_base;
		
	public:
		inline static constexpr double UNKNOWN_QUALITY{-1};
		typedef std::vector <metadata_filter const *>		filter_ptr_vector;
		
	protected:
		reader												*m_reader{};
		aligned_buffer <std::byte, buffer_base::zero_tag>	m_info{};			// Zero on copy b.c. the types may not be TriviallyCopyable.
		// FIXME: if the range contains only trivially copyable types, copy the bytes.
		filter_ptr_vector									m_filters{};
		std::vector <bool>									m_assigned_info_fields{};
		double												m_qual{UNKNOWN_QUALITY};
		std::size_t											m_variant_index{0};
		std::size_t											m_lineno{0};
		std::size_t											m_pos{0};
		
	public:
		// Make sure that both m_info and m_samples have zero return value for size(), see the formatters’ destructors.
		abstract_variant() = default;
		virtual ~abstract_variant() {}
		
		inline abstract_variant(reader &vcf_reader, std::size_t const info_size, std::size_t const info_alignment);
		
		void set_variant_index(std::size_t const idx)					{ m_variant_index = idx; }
		void set_lineno(std::size_t const lineno)						{ m_lineno = lineno; }
		void set_pos(std::size_t const pos)								{ m_pos = pos; }
		void set_qual(double const qual)								{ m_qual = qual; }
		
		class reader *reader() const									{ return m_reader; }
		filter_ptr_vector const &filters() const						{ return m_filters; }
		std::size_t variant_index() const								{ return m_variant_index; }
		std::size_t lineno() const										{ return m_lineno; }
		std::size_t pos() const											{ return m_pos; }
		inline std::size_t zero_based_pos() const;
		double qual() const												{ return m_qual; }
		
	protected:
		inline void reset();
	};
}

#endif

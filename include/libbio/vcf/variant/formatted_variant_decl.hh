/*
 * Copyright (c) 2020 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_VARIANT_FORMATTED_VARIANT_DECL_HH
#define LIBBIO_VCF_VARIANT_FORMATTED_VARIANT_DECL_HH

#include <libbio/vcf/variant/variant_tpl.hh>
#include <libbio/vcf/variant_format.hh>


namespace libbio {

	class vcf_reader;
}


namespace libbio { namespace detail {
	
	// Use static polymorphism to delegate construction, destruction and copying.
	class transient_variant_format_access
	{
		friend class abstract_variant;
		friend class variant_format_access;
		
		template <typename, typename>
		friend class formatted_variant_base;
		
	protected:
		transient_variant_format_access() = default;
		transient_variant_format_access(vcf_reader const &reader) {};
		inline variant_format_ptr const &get_format_ptr(vcf_reader const &reader) const;
	};
	
	
	class variant_format_access
	{
		friend class variant_base;
		
		template <typename, typename>
		friend class formatted_variant_base;

	protected:
		variant_format_ptr	m_format;
		
	protected:
		variant_format_access() = default;
		inline variant_format_access(vcf_reader const &reader);
		inline variant_format_access(vcf_reader const &reader, transient_variant_format_access const &other);
		
		variant_format_ptr const &get_format_ptr(vcf_reader const &reader) const { return m_format; }
	};
}}


namespace libbio {
	
	// Override constructors, destructor and copy assignment to delegate the operatrions.
	// Destructors of base classes are called in reverse order of declaration, so t_format_access’s destructor is called before variant_base’s.
	template <typename t_string, typename t_format_access>
	class formatted_variant : public variant_tpl <t_string>, public t_format_access
	{
		friend class vcf_reader;
		
	public:
		typedef variant_sample_tpl <t_string>	sample_type;
		
	public:
		formatted_variant() = default;
		~formatted_variant();
		
		inline formatted_variant(vcf_reader &reader, std::size_t const sample_count, std::size_t const info_size, std::size_t const info_alignment);
		
		// Copy constructors.
		inline formatted_variant(formatted_variant const &other);
		
		template <typename t_other_string, typename t_other_format_access>
		explicit inline formatted_variant(formatted_variant <t_other_string, t_other_format_access> const &other);
		
		// Move constructor.
		formatted_variant(formatted_variant &&) = default;
		
		// Copy assignment operators.
		inline formatted_variant &operator=(formatted_variant const &) &;
		
		template <typename t_other_string, typename t_other_format_access>
		inline formatted_variant &operator=(formatted_variant <t_other_string, t_other_format_access> const &) &;
		
		// Move assignment operator.
		formatted_variant &operator=(formatted_variant &&) & = default;
		
		inline variant_format_ptr const &get_format_ptr() const;
		inline variant_format const &get_format() const;
		
	protected:
		void initialize_info();
		void deinitialize_info();
		void initialize_samples();
		void deinitialize_samples();
		void reserve_memory_for_samples(
			std::uint16_t const size,
			std::uint16_t const alignment,
			std::uint16_t const field_count
		);
		
		template <typename t_other_string, typename t_other_format_access>
		void finish_copy(formatted_variant <t_other_string, t_other_format_access> const &other, bool should_initialize);
	};
}

#endif

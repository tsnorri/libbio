/*
 * Copyright (c) 2019 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_SUBFIELD_DECL_HH
#define LIBBIO_VCF_SUBFIELD_DECL_HH

#include <libbio/cxxcompat.hh>
#include <libbio/types.hh>
#include <libbio/utility.hh>
#include <libbio/vcf/vcf_metadata.hh>
#include <map>
#include <ostream>


namespace libbio {
	
	class variant_base;
	class variant_sample;
	
	
	// Base class and interface for field descriptions (specified by ##INFO, ##FORMAT).
	class vcf_subfield_base
	{
		friend class vcf_metadata_format;
		friend class vcf_metadata_formatted_field;
		
	public:
		virtual ~vcf_subfield_base() {}

		// Value type according to the VCF header.
		virtual vcf_metadata_value_type value_type() const = 0;
		
		// Get the metadata pointer.
		virtual vcf_metadata_base *get_metadata() const = 0;
		
	protected:
		// Alignment of this field.
		virtual std::uint16_t alignment() const = 0;
		
		// Number of items in this field according to the VCF header.
		virtual std::int32_t number() const = 0;
		
		// Mark the target s.t. no values have been read yet. (Currently used for vectors.)
		virtual void reset(std::byte *mem) const = 0;
		
		// Size of this field in bytes.
		virtual std::uint16_t byte_size() const = 0;
		
		// Construct the data structure.
		virtual void construct_ds(std::byte *mem, std::uint16_t const alt_count) const = 0;
		
		// Destruct the datastructure.
		virtual void destruct_ds(std::byte *mem) const = 0;
		
		// access_ds left out on purpose since its return type varies and it’s not needed for GT.
		
		// Copy the data structure.
		virtual void copy_ds(std::byte const *src, std::byte *dst) const = 0;
		
		// Create a clone of this object.
		virtual vcf_subfield_base *clone() const = 0;
		
		virtual inline std::uint16_t get_offset() const { return 0; }
		virtual inline void set_offset(std::uint16_t) {};
	};
	
	
	// Base class for info field descriptions.
	class vcf_info_field_base : public virtual vcf_subfield_base
	{
		friend class vcf_reader;
		
	protected:
		vcf_metadata_info	*m_metadata{};
		
	protected:
		// Parse the contents of a string_view and assign the value to the variant.
		// Needs to be overridden.
		virtual bool parse_and_assign(std::string_view const &sv, variant_base &var, std::byte *mem) const = 0;
		
		// Assign a value, used for FLAG.
		virtual bool assign(std::byte *mem) const { throw std::runtime_error("Not implemented"); };
		
		// For use with vcf_reader and variant classes:
		inline void prepare(variant_base &dst) const;
		inline void parse_and_assign(std::string_view const &sv, transient_variant &dst) const;
		inline void assign_flag(transient_variant &dst) const;
		virtual vcf_info_field_base *clone() const override = 0;
	public:
		vcf_metadata_info *get_metadata() const final { return m_metadata; }
		
		// Output the field contents to a stream. The value has to be present in the variant.
		virtual void output_vcf_value(std::ostream &stream, variant_base const &var) const = 0;
		
		// Output the given separator and the field contents to a stream if the value in present in the variant.
		bool output_vcf_value(std::ostream &stream, variant_base const &var, char const *sep) const;
		
		// Check whether the variant has a value for this INFO field.
		inline bool has_value(variant_base const &var) const;
	};
	
	
	// Base class for genotype field descriptions.
	class vcf_genotype_field_base : public virtual vcf_subfield_base
	{
		friend class vcf_reader;

	protected:
		vcf_metadata_format	*m_metadata{};
		
	protected:
		// Parse the contents of a string_view and assign the value to the sample.
		// Needs to be overridden.
		virtual bool parse_and_assign(std::string_view const &sv, variant_sample &sample, std::byte *mem) const = 0;
		
		// For use with vcf_reader and variant classes:
		inline void prepare(variant_sample &dst) const;
		inline void parse_and_assign(std::string_view const &sv, variant_sample &dst) const;
		virtual vcf_genotype_field_base *clone() const override = 0;
	public:
		vcf_metadata_format *get_metadata() const final { return m_metadata; }
		
		// Output the field contents to a stream.
		virtual void output_vcf_value(std::ostream &stream, variant_sample const &sample) const = 0;
	};
	
	
	// Base class for all field types that have an offset. (I.e. not GT.)
	class vcf_storable_subfield_base : public virtual vcf_subfield_base
	{
		friend class vcf_reader;
		
	public:
		enum { INVALID_OFFSET = UINT16_MAX };
		
	protected:
		std::uint16_t		m_offset{INVALID_OFFSET};	// Offset of this field in the memory block.
		
	protected:
		virtual inline std::uint16_t get_offset() const final { return m_offset; }
		virtual inline void set_offset(std::uint16_t offset) final { m_offset = offset; }
	};
	
	// Base classes for the different field types.
	class vcf_storable_info_field_base : public vcf_storable_subfield_base, public vcf_info_field_base
	{
		friend class vcf_metadata_info;
	};
	
	class vcf_storable_genotype_field_base : public vcf_storable_subfield_base, public vcf_genotype_field_base
	{
		friend class vcf_metadata_format;
	};
	
	
	// Base class for placeholders.
	class vcf_subfield_placeholder : public virtual vcf_subfield_base
	{
	public:
		virtual vcf_metadata_value_type value_type() const final { return vcf_metadata_value_type::NOT_PROCESSED; }
		
	protected:
		virtual std::uint16_t byte_size() const final { return 0; }
		virtual std::uint16_t alignment() const final { return 1; }
		virtual std::int32_t number() const final { return 0; }
		virtual void construct_ds(std::byte *mem, std::uint16_t const alt_count) const final { /* No-op. */ }
		virtual void destruct_ds(std::byte *mem) const final { /* No-op. */ }
		virtual void copy_ds(std::byte const *src, std::byte *dst) const final { /* No-op. */ }
		virtual void reset(std::byte *mem) const final { /* No-op. */ }
	};
	
	class vcf_info_field_placeholder : public vcf_info_field_base, public vcf_subfield_placeholder
	{
		virtual bool parse_and_assign(std::string_view const &sv, variant_base &var, std::byte *mem) const final { /* No-op. */ return false; }
		virtual vcf_info_field_placeholder *clone() const final { return new vcf_info_field_placeholder(*this); }
		
	public:
		virtual void output_vcf_value(std::ostream &stream, variant_base const &var) const final { throw std::runtime_error("Should not be called; parse_and_assign returns false"); }
	};
	
	class vcf_genotype_field_placeholder : public vcf_genotype_field_base, public vcf_subfield_placeholder
	{
		virtual bool parse_and_assign(std::string_view const &sv, variant_sample &sample, std::byte *mem) const final { /* No-op. */ return false; }
		virtual vcf_genotype_field_placeholder *clone() const final { return new vcf_genotype_field_placeholder(*this); }

	public:
		virtual void output_vcf_value(std::ostream &stream, variant_sample const &var) const final { throw std::runtime_error("Should not be called; parse_and_assign returns false"); }
	};
	
	
	typedef std::map <
		std::string,
		std::unique_ptr <vcf_info_field_base>,
		libbio::compare_strings_transparent
	>													vcf_info_field_map;
	typedef std::vector <vcf_info_field_base *>			vcf_info_field_ptr_vector;
	typedef std::map <
		std::string,
		std::unique_ptr <vcf_genotype_field_base>,
		libbio::compare_strings_transparent
	>													vcf_genotype_field_map;
	typedef std::vector <vcf_genotype_field_base *>		vcf_genotype_ptr_vector;
	
	void add_reserved_info_keys(vcf_info_field_map &dst);
	void add_reserved_genotype_keys(vcf_genotype_field_map &dst);
	
	// Fwd.
	template <
		std::int32_t t_number,
		vcf_metadata_value_type t_metadata_value_type,
		template <std::int32_t, vcf_metadata_value_type> class t_base
	>
	class vcf_generic_field;
	
	template <std::int32_t t_number, vcf_metadata_value_type t_metadata_value_type>
	class vcf_generic_info_field_base;
	
	template <std::int32_t t_number, vcf_metadata_value_type t_metadata_value_type>
	class vcf_generic_genotype_field_base;
	
	template <std::int32_t t_number, vcf_metadata_value_type t_metadata_value_type>
	using vcf_info_field = vcf_generic_field <t_number, t_metadata_value_type, vcf_generic_info_field_base>;

	template <std::int32_t t_number, vcf_metadata_value_type t_metadata_value_type>
	using vcf_genotype_field = vcf_generic_field <t_number, t_metadata_value_type, vcf_generic_genotype_field_base>;
	
	// Info fields.
	using vcf_info_field_aa			= vcf_info_field <1,										vcf_metadata_value_type::STRING>;
	using vcf_info_field_ac			= vcf_info_field <VCF_NUMBER_ONE_PER_ALTERNATE_ALLELE,		vcf_metadata_value_type::INTEGER>;
	using vcf_info_field_ad			= vcf_info_field <VCF_NUMBER_ONE_PER_ALLELE,				vcf_metadata_value_type::INTEGER>;
	using vcf_info_field_adf		= vcf_info_field <VCF_NUMBER_ONE_PER_ALLELE,				vcf_metadata_value_type::INTEGER>;
	using vcf_info_field_adr		= vcf_info_field <VCF_NUMBER_ONE_PER_ALLELE,				vcf_metadata_value_type::INTEGER>;
	using vcf_info_field_af			= vcf_info_field <VCF_NUMBER_ONE_PER_ALTERNATE_ALLELE,		vcf_metadata_value_type::FLOAT>;
	using vcf_info_field_an			= vcf_info_field <1,										vcf_metadata_value_type::INTEGER>;
	using vcf_info_field_bq			= vcf_info_field <1,										vcf_metadata_value_type::FLOAT>;
	using vcf_info_field_cigar		= vcf_info_field <VCF_NUMBER_ONE_PER_ALTERNATE_ALLELE,		vcf_metadata_value_type::STRING>;
	using vcf_info_field_db			= vcf_info_field <0,										vcf_metadata_value_type::FLAG>;
	using vcf_info_field_dp			= vcf_info_field <1,										vcf_metadata_value_type::INTEGER>;
	using vcf_info_field_end		= vcf_info_field <1,										vcf_metadata_value_type::INTEGER>;
	using vcf_info_field_h2			= vcf_info_field <0,										vcf_metadata_value_type::FLAG>;
	using vcf_info_field_h3			= vcf_info_field <0,										vcf_metadata_value_type::FLAG>;
	using vcf_info_field_mq			= vcf_info_field <1,										vcf_metadata_value_type::FLOAT>;
	using vcf_info_field_mq0		= vcf_info_field <1,										vcf_metadata_value_type::INTEGER>;
	using vcf_info_field_ns			= vcf_info_field <1,										vcf_metadata_value_type::INTEGER>;
	using vcf_info_field_sb			= vcf_info_field <4,										vcf_metadata_value_type::INTEGER>;
	using vcf_info_field_somatic	= vcf_info_field <0,										vcf_metadata_value_type::FLAG>;
	using vcf_info_field_validated	= vcf_info_field <0,										vcf_metadata_value_type::FLAG>;
	using vcf_info_field_1000g		= vcf_info_field <0,										vcf_metadata_value_type::FLAG>;
	
	// Genotype fields.
	using vcf_genotype_field_ad		= vcf_genotype_field <VCF_NUMBER_ONE_PER_ALLELE,			vcf_metadata_value_type::INTEGER>;
	using vcf_genotype_field_adf	= vcf_genotype_field <VCF_NUMBER_ONE_PER_ALLELE,			vcf_metadata_value_type::INTEGER>;
	using vcf_genotype_field_adr	= vcf_genotype_field <VCF_NUMBER_ONE_PER_ALLELE,			vcf_metadata_value_type::INTEGER>;
	using vcf_genotype_field_dp		= vcf_genotype_field <1,									vcf_metadata_value_type::INTEGER>;
	using vcf_genotype_field_ec		= vcf_genotype_field <VCF_NUMBER_ONE_PER_ALTERNATE_ALLELE,	vcf_metadata_value_type::INTEGER>;
	using vcf_genotype_field_ft		= vcf_genotype_field <1,									vcf_metadata_value_type::STRING>;
	using vcf_genotype_field_gl		= vcf_genotype_field <VCF_NUMBER_ONE_PER_GENOTYPE,			vcf_metadata_value_type::FLOAT>;
	using vcf_genotype_field_gp		= vcf_genotype_field <VCF_NUMBER_ONE_PER_GENOTYPE,			vcf_metadata_value_type::FLOAT>;
	using vcf_genotype_field_gq		= vcf_genotype_field <1,									vcf_metadata_value_type::INTEGER>;
	using vcf_genotype_field_hq		= vcf_genotype_field <2,									vcf_metadata_value_type::INTEGER>;
	using vcf_genotype_field_mq		= vcf_genotype_field <1,									vcf_metadata_value_type::INTEGER>;
	using vcf_genotype_field_pl		= vcf_genotype_field <VCF_NUMBER_ONE_PER_GENOTYPE,			vcf_metadata_value_type::INTEGER>;
	using vcf_genotype_field_pq		= vcf_genotype_field <1,									vcf_metadata_value_type::INTEGER>;
	using vcf_genotype_field_ps		= vcf_genotype_field <1,									vcf_metadata_value_type::INTEGER>;
}

#endif

/*
 * Copyright (c) 2022 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_REGION_VARIANT_VALIDATOR_HH
#define LIBBIO_VCF_REGION_VARIANT_VALIDATOR_HH

#include <libbio/bed_reader.hh>
#include <libbio/utility/compare_strings_transparent.hh>
#include <libbio/utility/string_hash.hh>
#include <libbio/vcf/vcf_reader_decl.hh>
#include <string>
#include <unordered_map>
#include <vector>


namespace libbio::vcf {
	
	struct position_range
	{
		std::size_t	begin{};
		std::size_t end{};
		
		position_range() = default;
		
		position_range(
			std::size_t const begin_,
			std::size_t const end_
		):
			begin(begin_),
			end(end_)
		{
		}
	};
	
	typedef std::vector <position_range> position_range_vector;
	
	struct position_range_cmp
	{
		// Compare the left bounds only.
		bool operator()(position_range const lhs, position_range const rhs) const { return lhs.begin < rhs.begin; }
	};
	
	
	struct region_state
	{
		position_range_vector	ranges;
		bool					is_seen{};
	};
	
	typedef std::unordered_map <
		std::string,
		region_state,
		libbio::string_hash_transparent,
		libbio::string_equal_to_transparent
	> region_state_map;
	
	
	class region_variant_validator : public variant_validator
	{
	protected:
		region_state_map				m_regions;
		position_range_vector::iterator	m_range_it{};
		position_range_vector::iterator	m_range_end{};
		std::string						m_prev_chr_id;
		std::size_t						m_prev_var_pos{};
		std::size_t						m_chr_id_mismatches{};
		std::size_t						m_position_mismatches{};
		bool							m_should_check_positions{};
		bool							m_is_known_region{};
		
	public:
		region_variant_validator(bool const should_check_positions):
			m_should_check_positions(should_check_positions)
		{
		}
		
		virtual ~region_variant_validator() {}
		
		region_state_map &regions() { return m_regions; }
		region_state_map const &regions() const { return m_regions; }
		std::size_t chromosome_id_mismatches() const { return m_chr_id_mismatches; }
		std::size_t position_mismatches() const { return m_position_mismatches; }
		
		variant_validation_result validate(transient_variant const &var) override;
		
	protected:
		// FIXME: Consider different designs, e.g. throwing if a variable is set.
		virtual variant_validation_result handle_unordered_contigs(transient_variant const &var) { return variant_validation_result::SKIP; }
		virtual variant_validation_result handle_unordered_variants(transient_variant const &var) { return variant_validation_result::SKIP; }
	};
	
	
	// Base class for a BED reader delegate.
	class region_variant_validator_bed_reader_delegate : public bed_reader_delegate // Not final.
	{
	protected:
		vcf::region_state_map *m_regions_by_chr_id{};
		
	public:
		region_variant_validator_bed_reader_delegate(region_state_map &regions_by_chr_id):
			m_regions_by_chr_id(&regions_by_chr_id)
		{
			regions_by_chr_id.clear();
		}
		
		virtual ~region_variant_validator_bed_reader_delegate() {}
		
		void bed_reader_found_region(std::string_view const chr_id, std::size_t const begin, std::size_t const end) override;
		void bed_reader_did_finish() override;
		
		// Define in a subclass:
		// void bed_reader_reported_error(std::size_t const lineno) override
	};
}

#endif

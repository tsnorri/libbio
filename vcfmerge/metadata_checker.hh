/*
 * Copyright (c) 2020 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCFMERGE_METADATA_CHECKER_HH
#define LIBBIO_VCFMERGE_METADATA_CHECKER_HH

#include <libbio/vcf/metadata.hh>
#include <map>
#include <ostream>
#include <range/v3/all.hpp>
#include <vector>
#include "vcf_input.hh"


namespace libbio::vcfmerge {
	
	template <typename t_metadata>
	struct metadata_item
	{
		vcf_input const		*source_input{};
		t_metadata const	*metadata{};
		
		metadata_item() = default;
		
		metadata_item(vcf_input const &source_input_, t_metadata const &metadata_):
			source_input(&source_input_),
			metadata(&metadata_)
		{
		}
	};
	
	
	struct metadata_sorter_base
	{
		virtual void sort_by_key(std::vector <vcf_input> const &inputs) = 0;
		virtual void output(std::ostream &os) const = 0;
	};
	
	
	struct metadata_checker_base
	{
		virtual bool check_metadata_required_matches(std::vector <vcf_input> const &inputs) const = 0;
		virtual bool check_metadata_optional_matches(std::vector <vcf_input> const &inputs) const = 0;
	};
	
	
	template <typename t_base>
	class metadata_sorter : public t_base, public metadata_sorter_base
	{
	public:
		typedef metadata_item <typename t_base::metadata_type>	item_type;
		typedef std::map <std::string, std::vector <item_type>>	metadata_map;
		
	protected:
		metadata_map m_metadata_by_key;
		
	public:
		metadata_map const &metadata_by_key() const { return m_metadata_by_key; }
		
		// Sort the expected metadata by key.
		void sort_by_key(std::vector <vcf_input> const &inputs) override
		{
			for (auto const &input : inputs)
			{
				auto const &all_meta(input.reader.metadata());
				auto const &specific_meta(this->access(all_meta));
				for (auto const &kv : specific_meta)
					m_metadata_by_key[kv.first].emplace_back(input, kv.second);
			}
		}
		
		void output(std::ostream &os) const override
		{
			for (auto const &kv : m_metadata_by_key)
			{
				libbio_assert(!kv.second.empty());
				kv.second.front().metadata->output_vcf(os);
			}
		}
	};
	
	
	template <typename t_base>
	class metadata_checker : public metadata_sorter <t_base>, public metadata_checker_base
	{
	public:
		typedef typename metadata_sorter <t_base>::item_type	item_type;
		
	protected:
		void report_mismatch(char const *reporting_level, item_type const &item1, item_type const &item2) const
		{
			std::cerr << reporting_level << ": The following " << this->name() << " metadata do not match in "
				<< item1.source_input->source_path << " and " << item2.source_input->source_path << ":\n";
		
			std::cerr << '\t';
			item1.metadata->output_vcf(std::cerr);
			std::cerr << "\n\t";
			item2.metadata->output_vcf(std::cerr);
			std::cerr << '\n';
		}
		
		
		// Compare.
		template <bool t_required_matches>
		bool check_metadata(std::vector <vcf_input> const &inputs) const
		{
			bool retval(true);
			
			// Verify that each equivalence class actually is one by doing
			// 1 == 2 && 2 == 3 && … && n == 1.
			for (auto const &kv : this->m_metadata_by_key)
			{
				if (1 < kv.second.size())
				{
					for (auto const &pair : kv.second | ranges::view::sliding(2))
					{
						auto const &item1(pair[0]);
						auto const &item2(pair[1]);
						
						bool status{};
						if constexpr (t_required_matches)
						{
							if (!this->compare_match_required(*item1.metadata, *item2.metadata))
							{
								retval = false;
								report_mismatch("ERROR", item1, item2);
							}
						}
						else
						{
							if (!this->compare_match_optional(*item1.metadata, *item2.metadata))
								report_mismatch("WARNING", item1, item2);
						}
					}
				}
			}
			
			return retval;
		}
		
		
	public:
		virtual bool check_metadata_required_matches(std::vector <vcf_input> const &inputs) const
		{
			return check_metadata <true>(inputs);
		}
		
		
		virtual bool check_metadata_optional_matches(std::vector <vcf_input> const &inputs) const
		{
			return check_metadata <false>(inputs);
		}
	};
	
	
	struct compare_contig
	{
		typedef vcf::metadata_contig metadata_type;
		char const *name() const { return "contig"; };
		auto const &access(vcf::metadata const &metadata) const { return metadata.contig(); }
		
		bool compare_match_required(metadata_type const &lhs, metadata_type const &rhs) const
		{
			return lhs.get_length() == rhs.get_length();
		}
		
		bool compare_match_optional(metadata_type const &lhs, metadata_type const &rhs) const
		{
			return true;
		}
	};
	
	
	struct compare_formatted
	{
		bool compare_match_required(vcf::metadata_formatted_field const &lhs, vcf::metadata_formatted_field const &rhs) const
		{
			return lhs.get_number() == rhs.get_number() && lhs.get_value_type() == rhs.get_value_type();
		}
		
		bool compare_match_optional(vcf::metadata_formatted_field const &lhs, vcf::metadata_formatted_field const &rhs) const
		{
			return lhs.get_description() == rhs.get_description();
		}
	};
	
	
	struct compare_info : public compare_formatted
	{
		typedef vcf::metadata_info metadata_type;
		char const *name() const { return "INFO"; }
		auto const &access(vcf::metadata const &metadata) const { return metadata.info(); }
		
		bool compare_match_optional(metadata_type const &lhs, metadata_type const &rhs) const
		{
			return compare_formatted::compare_match_optional(lhs, rhs) &&
				lhs.get_source() == rhs.get_source() &&
				lhs.get_version() == rhs.get_version();
		}
	};
	
	
	struct compare_format : public compare_formatted
	{
		typedef vcf::metadata_format metadata_type;
		char const *name() const { return "FORMAT"; }
		auto const &access(vcf::metadata const &metadata) const { return metadata.format(); }
	};
	
	
	struct compare_alt
	{
		typedef vcf::metadata_alt metadata_type;
		char const *name() const { return "ALT"; }
		auto const &access(vcf::metadata const &metadata) const { return metadata.alt(); }
		
		bool compare_match_required(metadata_type const &lhs, metadata_type const &rhs) const
		{
			return true;
		}
		
		bool compare_match_optional(metadata_type const &lhs, metadata_type const &rhs) const
		{
			return lhs.get_description() == rhs.get_description();
		}
	};
	
	
	struct compare_filter
	{
		typedef vcf::metadata_filter metadata_type;
		char const *name() const { return "FILTER"; }
		auto const &access(vcf::metadata const &metadata) const { return metadata.filter(); }
		
		bool compare_match_required(metadata_type const &lhs, metadata_type const &rhs) const
		{
			return true;
		}
		
		bool compare_match_optional(metadata_type const &lhs, metadata_type const &rhs) const
		{
			return lhs.get_description() == rhs.get_description();
		}
	};
}

#endif

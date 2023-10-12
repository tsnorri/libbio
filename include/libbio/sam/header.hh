/*
 * Copyright (c) 2023 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_SAM_HEADER_HH
#define LIBBIO_SAM_HEADER_HH

#include <libbio/sam/record.hh> // position_type
#include <string>
#include <vector>


namespace libbio::sam {
	
	enum class sort_order_type
	{
		unknown,
		unsorted,
		queryname,
		coordinate
	};
	
	
	enum class grouping_type
	{
		none,
		query,
		reference
	};
	
	
	enum class molecule_topology_type
	{
		unknown,
		linear,
		circular
	};
	
	
	struct reference_sequence_entry
	{
		std::string				name;
		position_type			length{};
		molecule_topology_type	molecule_topology{molecule_topology_type::linear};
		// FIXME: continue.
	};
	
	
	struct read_group_entry
	{
		std::string	id;
		std::string	description;
		// FIXME: continue.
	};
	
	
	struct program_entry
	{
		std::string	id;
		std::string	name;
		std::string	command_line;
		std::string	prev_id;
		std::string	description;
		std::string	version;
	};
	
	
	struct header
	{
		// FIXME: Add means for preserving header order (e.g. by adding a vector of pointers, common superclass for entries).
		
		typedef std::pair <
			std::string const *,
			std::uintptr_t
		>															reference_sequence_identifier_type;
		typedef std::vector <reference_sequence_identifier_type>	reference_sequence_identifier_vector;
		
		struct reference_sequence_identifier_cmp
		{
			bool operator()(reference_sequence_identifier_type const &lhs, std::string_view const &rhs) const { return *lhs.first < rhs; }
			bool operator()(std::string_view const &lhs, reference_sequence_identifier_type const &rhs) const { return lhs < *rhs.first; }
		};
		
		std::vector <reference_sequence_entry>	reference_sequences;
		std::vector <read_group_entry>			read_groups;
		std::vector <program_entry>				programs;
		std::vector <std::string>				comments;
		reference_sequence_identifier_vector	reference_sequence_identifiers;
		
		std::uint16_t	version_major{};
		std::uint16_t	version_minor{};
		
		sort_order_type	sort_order{sort_order_type::unknown};
		grouping_type	grouping{grouping_type::none};
		// FIXME: add subsorting.
		
		inline void clear();
		inline reference_id_type find_reference(std::string_view const sv) const;
	};
	
	
	char const *to_chars(sort_order_type const);
	char const *to_chars(grouping_type const);
	
	std::ostream &operator<<(std::ostream &os, sort_order_type const so) { os << to_chars(so); return os; }
	std::ostream &operator<<(std::ostream &os, grouping_type const go) { os << to_chars(go); return os; }
	std::ostream &operator<<(std::ostream &os, reference_sequence_entry const &ref_seq);
	std::ostream &operator<<(std::ostream &os, read_group_entry const &ref_seq);
	std::ostream &operator<<(std::ostream &os, program_entry const &ref_seq);
	std::ostream &operator<<(std::ostream &os, header const &);
	
	void output_record(std::ostream &os, header const &header_, record const &record);
	
	
	void header::clear()
	{
		reference_sequences.clear();
		read_groups.clear();
		programs.clear();
		comments.clear();
		reference_sequence_identifiers.clear();
	}
	
	
	reference_id_type header::find_reference(std::string_view const sv) const
	{
		auto const end(reference_sequence_identifiers.end());
		auto const it(std::lower_bound(reference_sequence_identifiers.begin(), end, sv, reference_sequence_identifier_cmp{}));
		if (it == end)
			return INVALID_REFERENCE_ID;
		if (*it->first != sv)
			return INVALID_REFERENCE_ID;
		return it->second;
	}
}

#endif

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
		molecule_topology_type	molecule_topology{molecule_topology_type::unknown}; // linear by default but we would like to preserve unset values.
		// FIXME: continue.
		
		template <typename t_reference_sequence_entry>
		static auto as_tuple_(t_reference_sequence_entry &&ee) { return std::tie(ee.name, ee.length, ee.molecule_topology); }
		
		auto as_tuple() { return as_tuple_(*this); }
		auto as_tuple() const { return as_tuple_(*this); }
		
		bool operator==(reference_sequence_entry const &other) const { return as_tuple() == other.as_tuple(); }
		
		template <typename t_string>
		reference_sequence_entry copy_and_rename(t_string &&name_) const;
	};
	
	typedef std::vector <reference_sequence_entry> reference_sequence_entry_vector;
	
	
	struct read_group_entry
	{
		std::string	id;
		std::string	description;
		// FIXME: continue.
	};
	
	typedef std::vector <read_group_entry> read_group_entry_vector;
	
	
	struct program_entry
	{
		std::string	id;
		std::string	name;
		std::string	command_line;
		std::string	prev_id;
		std::string	description;
		std::string	version;
	};
	
	typedef std::vector <program_entry> program_entry_vector;
	
	
	struct header
	{
		// FIXME: Add means for preserving header order (e.g. by adding a vector of pointers, common superclass for entries).
		
		typedef std::size_t											reference_sequence_identifier_type;
		typedef std::vector <reference_sequence_identifier_type>	reference_sequence_identifier_vector;
		
		struct reference_sequence_identifier_cmp
		{
			reference_sequence_entry_vector const &entries;
			
			bool operator()(reference_sequence_identifier_type const lhs, reference_sequence_identifier_type const rhs) const { return entries[lhs].name < entries[rhs].name; }
			bool operator()(reference_sequence_identifier_type const lhs, std::string_view const &rhs) const { return entries[lhs].name < rhs; }
			bool operator()(std::string_view const &lhs, reference_sequence_identifier_type const rhs) const { return lhs < entries[rhs].name; }
		};
		
		enum class copy_selection_type : std::uint16_t
		{
			reference_sequences	= 0x1,
			read_groups			= 0x2,
			programs			= 0x4,
			version				= 0x8,
			sort_order			= 0x10,
			comments			= 0x20
		};
		
		reference_sequence_entry_vector			reference_sequences;			// Sorted by index.
		read_group_entry_vector					read_groups;
		program_entry_vector					programs;
		std::vector <std::string>				comments;
		reference_sequence_identifier_vector	reference_sequence_identifiers;	// Sorted by name.
		
		std::uint16_t	version_major{};
		std::uint16_t	version_minor{};
		
		sort_order_type	sort_order{sort_order_type::unknown};
		grouping_type	grouping{grouping_type::none};
		// FIXME: add subsorting.
		
		inline void clear();
		inline reference_id_type find_reference(std::string_view const sv) const;
		
		template <copy_selection_type const t_fields>
		static header copy_subset(header const &other);
		
		void assign_reference_sequence_identifiers();
	};
	
	
	char const *to_chars(sort_order_type const);
	char const *to_chars(grouping_type const);
	
	inline std::ostream &operator<<(std::ostream &os, sort_order_type const so) { os << to_chars(so); return os; }
	inline std::ostream &operator<<(std::ostream &os, grouping_type const go) { os << to_chars(go); return os; }
	std::ostream &operator<<(std::ostream &os, reference_sequence_entry const &ref_seq);
	std::ostream &operator<<(std::ostream &os, read_group_entry const &ref_seq);
	std::ostream &operator<<(std::ostream &os, program_entry const &ref_seq);
	std::ostream &operator<<(std::ostream &os, header const &);
	
	void output_record(std::ostream &os, header const &header_, record const &record);
	void output_record_in_parsed_order(std::ostream &os, header const &header_, record const &record, std::vector <std::size_t> &buffer);
	
	
	constexpr inline header::copy_selection_type operator~(
		header::copy_selection_type val
	)
	{
		return static_cast <header::copy_selection_type>(~std::to_underlying(val));
	}
	
	constexpr inline header::copy_selection_type operator|(
		header::copy_selection_type const lhs,
		header::copy_selection_type const rhs
	)
	{
		return static_cast <header::copy_selection_type>(std::to_underlying(lhs) | std::to_underlying(rhs));
	}
	
	
	constexpr inline header::copy_selection_type operator&(
		header::copy_selection_type const lhs,
		header::copy_selection_type const rhs
	)
	{
		return static_cast <header::copy_selection_type>(std::to_underlying(lhs) & std::to_underlying(rhs));
	}
	
	
	constexpr inline bool any(header::copy_selection_type const fp)
	{
		return std::to_underlying(fp) ? true : false;
	}
	
	
	template <typename t_string>
	reference_sequence_entry reference_sequence_entry::copy_and_rename(t_string &&name_) const
	{
		reference_sequence_entry retval{.name = std::forward <t_string>(name_)};
		auto dst(retval.as_tuple());
		auto const src(as_tuple());
		tuples::for_ <std::tuple_size_v <decltype(src)>, 1>([&src, &dst]<typename t_idx> mutable {
			std::get <t_idx::value>(dst) = std::get <t_idx::value>(src);
		});
		return retval;
	}
	
	
	template <header::copy_selection_type const t_fields>
	header header::copy_subset(header const &other)
	{
		header retval;
		
		if constexpr (any(t_fields & copy_selection_type::reference_sequences))
		{
			retval.reference_sequences = other.reference_sequences;
			retval.reference_sequence_identifiers = other.reference_sequence_identifiers;
		}
		
		if constexpr (any(t_fields & copy_selection_type::read_groups))
			retval.read_groups = other.read_groups;
		
		if constexpr (any(t_fields & copy_selection_type::programs))
			retval.programs = other.programs;
		
		if constexpr (any(t_fields & copy_selection_type::version))
		{
			retval.version_major = other.version_major;
			retval.version_minor = other.version_minor;
		}
		
		if constexpr (any(t_fields & copy_selection_type::sort_order))
		{
			retval.sort_order = other.sort_order;
			retval.grouping = other.grouping;
		}
		
		if constexpr (any(t_fields & copy_selection_type::comments))
			retval.comments = other.comments;
		
		return retval;
	}
	
	
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
		auto const it(std::lower_bound(
			reference_sequence_identifiers.begin(),
			end,
			sv,
			reference_sequence_identifier_cmp{reference_sequences}
		));
		
		if (it == end)
			return INVALID_REFERENCE_ID;
		if (reference_sequences[*it].name != sv)
			return INVALID_REFERENCE_ID;
		return *it;
	}
}

#endif

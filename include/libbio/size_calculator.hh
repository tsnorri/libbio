/*
 * Copyright (c) 2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_SIZE_CALCULATOR_HH
#define LIBBIO_SIZE_CALCULATOR_HH

#include <cstdint>
#include <limits>
#include <ostream>
#include <string>
#include <vector>


namespace libbio {
	struct size_calculator;
	struct size_calculator_entry;
}


namespace libbio::size_calculation {

	typedef std::uint64_t	size_type;
	typedef std::uint64_t	entry_index_type;


	template <typename t_type>
	struct value_size_calculator
	{
		// Intended to be specialised.
	};

	template <typename t_type>
	void calculate_value_size(size_calculator &sc, entry_index_type const entry_idx, t_type const &val)
	{
		value_size_calculator <t_type> vsc;
		vsc(sc, entry_idx, val);
	}
}


namespace libbio {

	struct size_calculator_entry
	{
		typedef size_calculation::size_type			size_type;
		typedef size_calculation::entry_index_type	index_type;
		constexpr static inline index_type INVALID_ENTRY{std::numeric_limits <index_type>::max()};

		std::string	name;
		size_type	size{};
		index_type	parent{INVALID_ENTRY};
	};


	struct size_calculator
	{
		typedef size_calculator_entry	entry;
		typedef entry::index_type		entry_index_type;
		
		struct add_entry_return_type
		{
			size_calculator_entry		&entry;
			entry_index_type			index{};
		};

		std::vector <entry>	entries;

		add_entry_return_type add_root_entry();
		add_entry_return_type add_entry(entry_index_type const parent_idx);

		template <typename t_type>
		add_entry_return_type add_entry_for(entry_index_type const parent_idx, char const *name, t_type const &val);

		void sum_sizes();
		void output_entries(std::ostream &os);
	};


	template <typename t_type>
	auto size_calculator::add_entry_for(entry_index_type const parent_idx, char const *name, t_type const &val) -> add_entry_return_type
	{
		auto retval(add_entry(parent_idx));
		retval.entry.name = name;
		size_calculation::calculate_value_size(*this, retval.index, val);
		return retval;
	}
}


namespace libbio::size_calculation {

	// Partial specialisation for std::vector.
	template <typename t_type>
	struct value_size_calculator <std::vector <t_type>>
	{
		void operator()(size_calculator &sc, entry_index_type const entry_idx, std::vector <t_type> const &vec) const
		{
			sc.entries[entry_idx].size += vec.size() * sizeof(t_type) + sizeof(std::size_t); // Best guess.
		}
	};
}

#endif

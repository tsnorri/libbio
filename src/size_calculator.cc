/*
 * Copyright (c) 2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <libbio/assert.hh>
#include <libbio/size_calculator.hh>
#include <range/v3/view/enumerate.hpp>
#include <range/v3/view/reverse.hpp>

namespace rsv	= ranges::views;


namespace libbio {

	auto size_calculator::add_root_entry() -> add_entry_return_type
	{
		entry *root_entry{};
		if (entries.empty())
		{
			root_entry = &entries.emplace_back();
			root_entry->name = "<root>";
		}
		else
		{
			root_entry = &entries.front();
		}

		return {*root_entry, 0};
	}


	auto size_calculator::add_entry(entry_index_type const parent_idx) -> add_entry_return_type
	{
		auto const idx{entries.size()};
		add_entry_return_type retval{entries.emplace_back(), idx};
		retval.entry.parent = parent_idx;
		return retval;
	}


	void size_calculator::sum_sizes()
	{
		for (auto const &entry : rsv::reverse(entries))
		{
			if (entry::INVALID_ENTRY != entry.parent)
			{
				libbio_assert_lt(entry.parent, entries.size());
				entries[entry.parent].size += entry.size;
			}
		}
	}


	void size_calculator::output_entries(std::ostream &os)
	{
		os << "ENTRY\tPARENT\tNAME\tSIZE\n";
		for (auto const &[idx, entry] : rsv::enumerate(entries))
			os << (1 + idx) << '\t' << (entry.parent + 1) << '\t' << entry.name << '\t' << entry.size << '\n';
	}
}

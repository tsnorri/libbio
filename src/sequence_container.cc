/*
 * Copyright (c) 2017 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <libbio/sequence_reader/sequence_container.hh>


namespace libbio { namespace sequence_reader {
	
	void mmap_sequence_container::to_spans(sequence_vector &dst)
	{	
		auto const sv(m_handle.to_string_view());
		auto const limit(sv.size());
		decltype(sv)::size_type pos(0), next_pos(0);
		while (true)
		{
			next_pos = sv.find_first_of('\n', pos);
			if (next_pos == decltype(sv)::npos)
				break;
			
			dst.emplace_back(reinterpret_cast <std::uint8_t const *>(sv.data() + pos), next_pos - pos);
			
			pos = next_pos;
		}
	}
}}

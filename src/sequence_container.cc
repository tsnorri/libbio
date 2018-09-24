/*
 * Copyright (c) 2017 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <libbio/sequence_reader/sequence_container.hh>
#include <ostream>


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
	
	
	std::ostream &operator<<(std::ostream &stream, vector_sequence_container const &container)
	{
		stream << container.m_sequences.size() << " sequence vectors";
		return stream;
	}
	
	
	std::ostream &operator<<(std::ostream &stream, mmap_sequence_container const &container)
	{
		stream << "Handle: " << container.m_handle << " sequence length: " << container.m_sequence_length << " sequence count: " << container.m_sequence_count;
		return stream;
	}
	
	
	std::ostream &operator<<(std::ostream &stream, multiple_mmap_sequence_container const &container)
	{
		stream << "Mapped files:\n";
		for (auto const &handle : container.m_handles)
			stream << '\t' << handle << '\n';
		return stream;
	}
}}

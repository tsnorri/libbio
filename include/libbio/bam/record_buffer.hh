/*
 * Copyright (c) 2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_BAM_RECORD_BUFFER_HH
#define LIBBIO_BAM_RECORD_BUFFER_HH

#include <cstddef>
#include <libbio/sam/record.hh>
#include <vector>


namespace libbio::bam {

	class record_buffer
	{
	public:
		typedef std::vector <sam::record>		record_vector;
		typedef record_vector::iterator			iterator;
		typedef record_vector::const_iterator	const_iterator;

	private:
		record_vector	m_records;
		std::size_t		m_size{};

	public:
		inline sam::record &next_record();
		void clear() { m_size = 0; }
		iterator begin() { return m_records.begin(); }
		iterator end() { return m_records.begin() + m_size; }
		const_iterator begin() const { return m_records.begin(); }
		const_iterator end() const { return m_records.begin() + m_size; }
		const_iterator cbegin() const { return m_records.begin(); }
		const_iterator cend() const { return m_records.begin() + m_size; }
	};


	sam::record &record_buffer::next_record()
	{
		if (m_size < m_records.size())
			return m_records[m_size++];

		++m_size;
		return m_records.emplace_back();
	}
}

#endif

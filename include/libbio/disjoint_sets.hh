/*
 * Copyright (c) 2026 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_DISJOINT_SETS_HH
#define LIBBIO_DISJOINT_SETS_HH

#include <numeric>


namespace libbio {

	template <typename t_vector>
	class disjoint_sets
	{
	public:
		typedef t_vector vector_type;
		typedef vector_type::value_type value_type;

	private:
		vector_type m_parents{};
		vector_type m_sizes{}; // TODO: union by rank?

	public:
		disjoint_sets() = default;

		explicit disjoint_sets(value_type const count):
			m_parents(count),
			m_sizes(count, 1)
		{
			std::iota(m_parents.begin(), m_parents.end(), 0);
		}

		bool is_singleton(value_type node) const { return m_parents[node] == node; }
		value_type find(value_type node);
		void set_union(value_type n1, value_type n2);
	};


	template <typename t_vector>
	auto disjoint_sets <t_vector>::find(value_type node) -> value_type
	{
		value_type retval{node};
		while (m_parents[retval] != retval)
			retval = m_parents[node];

		while (m_parents[node] != node)
		{
			auto const node_{node};
			node = m_parents[node];
			m_parents[node_] = retval;
		}

		return retval;
	}


	template <typename t_vector>
	void disjoint_sets <t_vector>::set_union(value_type n1, value_type n2)
	{
		n1 = find(n1);
		n2 = find(n2);

		if (n1 == n2) return;

		if (m_sizes[n1] < m_sizes[n2])
		{
			using std::swap;
			swap(n1, n2);
		}

		// n1 is the new root.
		m_parents[n2] = n1;
		m_sizes[n1] += m_sizes[n2];
	}
}

#endif

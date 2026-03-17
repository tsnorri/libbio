/*
 * Copyright (c) 2026 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_ACCUMULATOR_SUM_HH
#define LIBBIO_ACCUMULATOR_SUM_HH

#include <algorithm>
#include <functional>


namespace libbio::accumulators::detail {

	template <typename t_value>
	struct min_cmp
	{
		typedef std::less <t_value> less_type;

		bool operator()(t_value const &lhs, t_value const &rhs) const { return std::min(lhs, rhs, less_type{}); }
	};


	// I think we could parametrise min_cmp with std::greater but this is more readable.
	template <typename t_value>
	struct max_cmp
	{
		typedef std::less <t_value> less_type;

		bool operator()(t_value const &lhs, t_value const &rhs) const { return std::max(lhs, rhs, less_type{}); }
	};
}


namespace libbio::accumulators {

	template <typename t_value, typename t_plus = std::plus <t_value>, typename t_pairwise_plus = std::plus <t_value>>
	class sum_tpl
	{
	public:
		typedef t_value value_type;

	private:
		value_type m_sum{};

	public:
		void init(value_type value);
		void update(value_type value);
		void pairwise_update(sum_tpl const &other);

		value_type value() const { return m_sum; }
	};


	template <typename t_value, typename t_plus, typename t_pairwise_plus>
	void sum_tpl <t_value, t_plus, t_pairwise_plus>::init(value_type value)
	{
		m_sum = value;
	}


	template <typename t_value, typename t_plus, typename t_pairwise_plus>
	void sum_tpl <t_value, t_plus, t_pairwise_plus>::update(value_type value)
	{
		m_sum = t_plus{}(m_sum, value);
	}


	template <typename t_value, typename t_plus, typename t_pairwise_plus>
	void sum_tpl <t_value, t_plus, t_pairwise_plus>::pairwise_update(
		sum_tpl const &other
	)
	{
		m_sum = t_pairwise_plus{}(m_sum, other.m_sum);
	}


	template <typename t_value>
	using sum = sum_tpl <t_value>;

	template <typename t_value>
	using min_sum = sum_tpl <t_value, std::plus <t_value>, detail::min_cmp <t_value>>;

	template <typename t_value>
	using max_sum = sum_tpl <t_value, std::plus <t_value>, detail::max_cmp <t_value>>;
}

#endif

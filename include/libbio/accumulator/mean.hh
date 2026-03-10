/*
 * Copyright (c) 2026 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_ACCUMULATOR_MEAN_HH
#define LIBBIO_ACCUMULATOR_MEAN_HH

#include <cstddef>


namespace libbio::accumulators {

	// Based on:
	// J. Bennett, R. Grout, P. Pebay, D. Roe and D. Thompson, "Numerically stable, single-pass, parallel statistics algorithms," 2009 IEEE International Conference on Cluster Computing and Workshops, New Orleans, LA, USA, 2009, pp. 1-8, doi: 10.1109/CLUSTR.2009.5289161.
	template <typename t_value>
	class mean
	{
	public:
		typedef t_value value_type;

	private:
		value_type m_mu{};
		std::size_t m_i{};

	public:
		void init(value_type value);
		void update(value_type value);
		void pairwise_update(mean const &other);

		std::size_t size() const { return m_i; }
		bool empty() const { return 0 == size(); }

		value_type value() const { return m_mu; }
	};


	template <typename t_value>
	void mean <t_value>::init(value_type value)
	{
		m_mu = value;
		m_i = 1;
	}


	template <typename t_value>
	void mean <t_value>::update(value_type value)
	{
		// Update the mean (Bennett et al. Equation II.1).
		++m_i;
		m_mu += (value - m_mu) / m_i;
	}


	template <typename t_value>
	void mean <t_value>::pairwise_update(
		mean const &other
	)
	{
		auto const count_sum{m_i + other.m_i};

		// Update the mean (Bennett et al. Equation II.3).
		auto const delta{other.m_mu - m_mu};
		m_mu += value_type(other.m_i) / count_sum * delta;

		// Update the count.
		m_i = count_sum;
	}
}

#endif

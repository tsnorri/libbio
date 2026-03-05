/*
 * Copyright (c) 2026 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_PEARSON_CORRELATION_HH
#define LIBBIO_PEARSON_CORRELATION_HH

#include <cmath>
#include <cstddef>
#include <limits>


namespace libbio {

	// Based on:
	// Welford, B. P. (1962). Note on a Method for Calculating Corrected Sums of Squares and Products. Technometrics, 4(3), 419–420. https://doi.org/10.1080/00401706.1962.10490022
	// J. Bennett, R. Grout, P. Pebay, D. Roe and D. Thompson, "Numerically stable, single-pass, parallel statistics algorithms," 2009 IEEE International Conference on Cluster Computing and Workshops, New Orleans, LA, USA, 2009, pp. 1-8, doi: 10.1109/CLUSTR.2009.5289161.
	template <typename t_value>
	class pearson_correlation_coefficient_calculator
	{
	public:
		typedef t_value value_type;

	private:
		value_type m_mu_lhs{};
		value_type m_mu_rhs{};
		value_type m_s_lhs{};
		value_type m_s_rhs{};
		value_type m_c{};
		std::size_t m_i{};

	public:
		void init(value_type lhs, value_type rhs);
		void update(value_type lhs, value_type rhs);
		void pairwise_update(pearson_correlation_coefficient_calculator const &other);

		std::size_t size() const { return m_i; }
		bool empty() const { return 0 == size(); }

		value_type correlation() const;
	};


	template <typename t_value>
	void pearson_correlation_coefficient_calculator <t_value>::init(value_type lhs, value_type rhs)
	{
		m_mu_lhs = lhs;
		m_mu_rhs = rhs;
		m_s_lhs = 0;
		m_s_rhs = 0;
		m_c = 0;
		m_i = 1;
	}


	template <typename t_value>
	void pearson_correlation_coefficient_calculator <t_value>::update(value_type lhs, value_type rhs)
	{
		value_type const lhs_{lhs - m_mu_lhs};
		value_type const rhs_{rhs - m_mu_rhs};

		// Update the means (Bennett et al. Equation II.1).
		m_mu_lhs += lhs_ / (m_i + 1);
		m_mu_rhs += rhs_ / (m_i + 1);

		// Update the sums of squares for calculating standard deviation (Welford, Equation I).
		// Knuth also gives S_{k+1} = S_k + (x_{k+1} - mu_k)(x_{k+1} - mu_{k+1})
		m_s_lhs += (m_i * lhs_ * lhs_) / (m_i + 1);
		m_s_rhs += (m_i * rhs_ * rhs_) / (m_i + 1);

		// Update the sum of squares for calculating covariance (Bennett et al. Equation III.9).
		m_c += m_i * lhs_ * rhs_ / (m_i + 1);

		++m_i;
	}


	template <typename t_value>
	void pearson_correlation_coefficient_calculator <t_value>::pairwise_update(
		pearson_correlation_coefficient_calculator const &other
	)
	{
		auto const delta_lhs{other.m_mu_lhs - m_mu_lhs};
		auto const delta_rhs{other.m_mu_rhs - m_mu_rhs};
		auto const count_sum{m_i + other.m_i};
		auto const count_multiplier{(value_type(m_i) * value_type(other.m_i)) / value_type(count_sum)};

		// Update the means (Bennett et al. Equation II.3).
		m_mu_lhs += value_type(other.m_i) / count_sum * delta_lhs;
		m_mu_rhs += value_type(other.m_i) / count_sum * delta_rhs;

		// Update the sums of squares for calculating standard deviation (Bennett et al. Equation II.4).
		m_s_lhs += other.m_s_lhs + delta_lhs * delta_lhs * count_multiplier;
		m_s_rhs += other.m_s_rhs + delta_rhs * delta_rhs * count_multiplier;

		// Update the sum of squares for calculating covariance (Bennett et al. Equation III.6).
		m_c += other.m_c + delta_lhs * delta_rhs * count_multiplier;

		// Update the count.
		m_i = count_sum;
	}


	template <typename t_value>
	auto pearson_correlation_coefficient_calculator <t_value>::correlation() const -> value_type
	{
		if (0 == m_s_lhs || 0 == m_s_rhs)
			return std::numeric_limits <value_type>::quiet_NaN();

		using std::sqrt;
		auto const rho{m_c / sqrt(m_s_lhs * m_s_rhs)};
		if (1 < rho) return 1;
		if (rho < -1) return -1;
		return rho;
	}
}

#endif

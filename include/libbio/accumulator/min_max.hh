/*
 * Copyright (c) 2026 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_ACCUMULATOR_MIN_MAX_HH
#define LIBBIO_ACCUMULATOR_MIN_MAX_HH

#include <algorithm>
#include <functional>


namespace libbio::accumulators {

	template <typename t_value, typename t_cmp = std::less <t_value>>
	class min_tpl
	{
	public:
		typedef t_value value_type;

	private:
		value_type m_min{};

	public:
		void init(value_type value);
		void update(value_type value);
		void pairwise_update(min_tpl const &other);

		value_type value() const { return m_min; }
	};


	template <typename t_value, typename t_cmp>
	void min_tpl <t_value, t_cmp>::init(value_type value)
	{
		m_min = value;
	}


	template <typename t_value, typename t_cmp>
	void min_tpl <t_value, t_cmp>::update(value_type value)
	{
		m_min = std::min(m_min, value, t_cmp{});
	}


	template <typename t_value, typename t_cmp>
	void min_tpl <t_value, t_cmp>::pairwise_update(
		min_tpl const &other
	)
	{
		m_min = std::min(m_min, other.m_min, t_cmp{});
	}


	template <typename t_value>
	using min = min_tpl <t_value, std::less <t_value>>;

	template <typename t_value>
	using max = min_tpl <t_value, std::greater <t_value>>;
}

#endif

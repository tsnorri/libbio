/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_RLE_BIT_VECTOR_HH
#define LIBBIO_RLE_BIT_VECTOR_HH

#include <libbio/algorithm.hh>
#include <range/v3/algorithm/copy.hpp>
#include <range/v3/view/sliding.hpp>
#include <range/v3/view/transform.hpp>
#include <vector>

namespace libbio { namespace detail {
	
	template <typename t_rle_bit_vector, bool t_is_const = std::is_const_v <t_rle_bit_vector>>
	struct rle_bit_vector_iterator_traits
	{
		typedef typename t_rle_bit_vector::run_iterator					run_iterator;
		typedef typename t_rle_bit_vector::reverse_run_iterator			reverse_run_iterator;
	};
	
	
	template <typename t_rle_bit_vector>
	struct rle_bit_vector_iterator_traits <t_rle_bit_vector, true>
	{
		typedef typename t_rle_bit_vector::const_run_iterator			run_iterator;
		typedef typename t_rle_bit_vector::const_reverse_run_iterator	reverse_run_iterator;
	};
	
	
	template <typename t_rle_bit_vector>
	class run_iterator_proxy
	{
	public:
		typedef rle_bit_vector_iterator_traits <t_rle_bit_vector>		traits;
		typedef	typename traits::run_iterator							run_iterator;
		typedef	typename traits::reverse_run_iterator					reverse_run_iterator;
		typedef typename t_rle_bit_vector::const_run_iterator			const_run_iterator;
		typedef typename t_rle_bit_vector::const_reverse_run_iterator	const_reverse_run_iterator;
		
	protected:
		t_rle_bit_vector	*m_rle_vector{};
		
	public:
		run_iterator_proxy() = default;
		run_iterator_proxy(t_rle_bit_vector &vector):
			m_rle_vector(&vector)
		{
		}
		
		bool starts_with_zero() const { return m_rle_vector->m_starts_with_zero; }
		
		run_iterator begin() const { return m_rle_vector->m_values.begin(); }
		run_iterator end() const { return m_rle_vector->m_values.end(); }
		reverse_run_iterator rbegin() const { return m_rle_vector->m_values.rbegin(); }
		reverse_run_iterator rend() const { return m_rle_vector->m_values.rend(); }
		
		// FIXME: are these needed?
		const_run_iterator cbegin() const { return m_rle_vector->m_values.cbegin(); }
		const_run_iterator cend() const { return m_rle_vector->m_values.cend(); }
		const_reverse_run_iterator crbegin() const { return m_rle_vector->m_values.crbegin(); }
		const_reverse_run_iterator crend() const { return m_rle_vector->m_values.crend(); }
	};
}}


namespace libbio {
	
	template <typename t_count>
	class rle_bit_vector
	{
		friend detail::run_iterator_proxy <rle_bit_vector>;
		friend detail::run_iterator_proxy <rle_bit_vector const>;
		
	public:
		typedef t_count												count_type;
		typedef std::vector <count_type>							vector_type;
		
		typedef typename vector_type::iterator						run_iterator;
		typedef typename vector_type::const_iterator				const_run_iterator;
		typedef typename vector_type::reverse_iterator				reverse_run_iterator;
		typedef typename vector_type::const_reverse_iterator		const_reverse_run_iterator;
		
		typedef detail::run_iterator_proxy <rle_bit_vector>			run_iterator_proxy;
		typedef detail::run_iterator_proxy <rle_bit_vector const>	const_run_iterator_proxy;
		
	protected:
		vector_type	m_values;
		bool		m_starts_with_zero{true};
		
	public:
		rle_bit_vector() = default;
		
		bool starts_with_zero() const { return m_starts_with_zero; }
		
		run_iterator_proxy runs() const { return run_iterator_proxy(*this); }
		const_run_iterator_proxy const_runs() const { return const_run_iterator_proxy(*this); }
		
		void push_back(bool val, count_type count = 1);
		void reverse();
		void clear() { m_values.clear(); }
		
		// For Python, re-declaring aliased iterators is tedious.
		vector_type const &to_run_vector() const { return m_values; }
	};
	
	
	template <typename t_count>
	void rle_bit_vector <t_count>::push_back(bool val, count_type const count)
	{
		val &= 0x1;
		
		if (0 == m_values.size())
		{
			if (val)
				m_starts_with_zero = false;
			m_values.push_back(count);
		}
		else
		{
			bool const last_value((m_values.size() % 2) ^ m_starts_with_zero); // Same as addition.
			if (val ^ last_value)
				m_values.push_back(count);
			else
				*m_values.rbegin() += count;
		}
	}
	
	
	template <typename t_count>
	void rle_bit_vector <t_count>::reverse()
	{
		std::reverse(m_values.begin(), m_values.end());
		if (0 == m_values.size() % 2)
			m_starts_with_zero = !m_starts_with_zero;
	}
}

#endif

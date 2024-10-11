/*
 * Copyright (c) 2018-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_PBWT_HH
#define LIBBIO_PBWT_HH

#include <algorithm>
#include <cstddef>
#include <libbio/array_list.hh>
#include <range/v3/all.hpp>


namespace libbio::pbwt {
	
	template <typename t_permutation_vector, typename t_divergence_vector>
	class dynamic_pbwt_rmq
	{
	public:
		typedef typename t_divergence_vector::size_type	size_type;
		
	protected:
		t_permutation_vector	*m_permutation{};
		t_divergence_vector		*m_divergence{};
		
	public:
		dynamic_pbwt_rmq() = default;
		dynamic_pbwt_rmq(t_permutation_vector &input_permutation, t_divergence_vector &input_divergence):
			m_permutation(&input_permutation),
			m_divergence(&input_divergence)
		{
		}

		size_type size() const { return m_divergence->size(); }
		void update(std::size_t i) { (*m_permutation)[i] = i + 1; }
		size_type operator()(size_type j, size_type i) { return maxd(j, i); }
		
	protected:
		size_type maxd(size_type j, size_type i)
		{
			if (j != i)
			{
				auto const lhs((*m_divergence)[j]);
				auto const p((*m_permutation)[j]);
				auto const rhs(maxd(p, i));
				(*m_divergence)[j] = max_ct(lhs, rhs);
				(*m_permutation)[j] = i + 1;
			}
			
			return (*m_divergence)[j];
		}
	};
	
	
	// Build prefix and divergence arrays for PBWT.
	template <
		typename t_input_vector,
		typename t_alphabet,
		typename t_string_index_vector,
		typename t_character_index_vector,
		typename t_ci_rmq,
		typename t_count_vector,
		typename t_previous_position_vector
	>
	void build_prefix_and_divergence_arrays(
		t_input_vector const &inputs,						// Vector of input vectors.
		std::size_t const column_idx,						// Current column index, k.
		t_alphabet const &alphabet,							// Object for converting the input characters to consecutive integers.
		t_string_index_vector const &input_permutation,		// Order in which the input is iterated, a_k.
		t_character_index_vector const &input_divergence,	// Input divergence, d_k.
		t_ci_rmq &input_divergence_rmq,						// Range maximum query support for d_k. dynamic_pbwt_rmq requires mutability.
		t_string_index_vector &sorted_permutation,			// Sorted indexes, a_{k + 1}.
		t_character_index_vector &output_divergence,		// Output divergence, d_{k + 1}.
		t_count_vector &counts,								// Character counts.
		t_previous_position_vector &previous_positions		// Previous position of each character.
	)
	{
		auto const input_count(inputs.size());
		auto const sigma(alphabet.sigma());
		
		// Sanity checks.
		libbio_assert(input_count == input_permutation.size());
		libbio_assert(input_count == input_divergence.size());
		libbio_assert(input_count == input_divergence_rmq.size());
		libbio_assert(sigma <= previous_positions.size());
		libbio_assert(sigma <= counts.size());
		
		// Count the instances of each character after zero-filling the count array.
		std::fill(counts.begin(), sigma + counts.begin(), 0);
		for (auto const vec_idx : input_permutation)
		{
			auto const &vec(inputs[vec_idx]);
			auto const c(vec[column_idx]);
			auto const comp(alphabet.char_to_comp(c));
			++counts[comp];
		}
		
		// Calculate the cumulative sum.
		{
			typename t_count_vector::value_type sum(0);
			for (auto &c : counts)
			{
				c += sum;
				
				using std::swap;
				swap(c, sum);
			}
		}
		
		// Sort the strings by the k-th column and build the arrays.
		if (previous_positions.size() < 1 + sigma)
			previous_positions.resize(1 + sigma);
		std::fill(previous_positions.begin(), 1 + sigma + previous_positions.begin(), 0);
		{
			std::size_t i(0);									// j in Algorithm 2.1.
			for (auto const vec_idx : input_permutation)		// a_k[j] in Algorithm 2.1.
			{
				auto const &vec(inputs[vec_idx]);
				auto const c(vec[column_idx]);
				auto const comp(alphabet.char_to_comp(c));
				auto const dst_idx(counts[comp]++);				// j' in Algorithm 2.1.
				sorted_permutation[dst_idx] = vec_idx;			// Store the identifier of the current string to a_{k + 1}.

				// Update the RMQ data structure if needed.
				input_divergence_rmq.update(i);
			
				// Next value for d_{k + 1}.
				auto const prev_idx(previous_positions[comp]);	// i in Algorithm 2.1.
				libbio_assert(prev_idx <= i);
				if (0 == prev_idx)
					output_divergence[dst_idx] = 1 + column_idx;
				else
				{
					auto const max_div(input_divergence_rmq(input_divergence, prev_idx, i));
					output_divergence[dst_idx] = max_div;
				}
				previous_positions[comp] = 1 + i;
				++i;
			}
		}
	}
	
	
	template <typename t_string_index_vector>
	void update_inverse_input_permutation(
		t_string_index_vector const &input_permutation,
		t_string_index_vector &inverse_input_permutation
	)
	{
		std::size_t idx(0);
		for (auto const val : input_permutation)
			inverse_input_permutation[val] = idx++;
	}
	
	
	template <
		typename t_character_index_vector,
		typename t_divergence_count_type
	>
	void update_divergence_value_counts(
		t_character_index_vector const &input_divergence,
		t_character_index_vector const &output_divergence,
		array_list <t_divergence_count_type> &count_list
	)
	{
		for (auto const output_val : output_divergence)
		{
			libbio_assert(output_val < count_list.size());
			
			// Add the new value. If the value did not occur in the list before,
			// it comes from the current column.
			auto const last_idx_1(count_list.last_index_1());
			if (last_idx_1 <= output_val)
			{
				libbio_assert(0 == count_list[output_val]);
				count_list.link(1, output_val, last_idx_1 - 1);
			}
			else
			{
				auto &count(count_list[output_val]);
				libbio_assert(count);
				++count;
			}
		}
		
		for (auto const input_val : input_divergence)
		{
			libbio_assert(input_val < count_list.size());
			
			// Decrement the old value and remove if necessary.
			auto input_it(count_list.find(input_val));
			libbio_assert(count_list.end() != input_it);
			auto &count(*input_it);
			libbio_assert(0 < count);
			if (0 == --count)
				count_list.erase(input_it, false);
		}
	}
	
	
	template <
		typename t_position,
		typename t_divergence
	>
	std::size_t unique_substring_count(t_position const lb, t_divergence const &divergence)
	{
		std::size_t retval(0);
		for (auto const dd : divergence)
		{
			if (lb < dd)
				++retval;
		}
	
		return retval;
	}
	
	
	// Index by occurrence (0, 1, 2, …) in PBWT order.
	template <
		typename t_position,
		typename t_divergence,
		typename t_counts
	>
	std::size_t unique_substring_count(t_position const lb, t_divergence const &divergence, t_counts &substring_copy_numbers)
	{
		substring_copy_numbers.clear();
		
		std::size_t run_cn(1);
		std::size_t idx(0);
		for (auto const dd : divergence | ranges::view::drop(1))
		{
			if (dd <= lb)
				++run_cn;
			else
			{
				substring_copy_numbers.emplace_back(idx++, run_cn);
				run_cn = 1;
			}
		}
		substring_copy_numbers.emplace_back(idx++, run_cn);
		return idx;
	}
	
	
	// Index by string number in PBWT order.
	template <
		typename t_position,
		typename t_string_indices,
		typename t_divergence,
		typename t_counts
	>
	std::size_t unique_substring_count_idxs(t_position const lb, t_string_indices const &permutation, t_divergence const &divergence, t_counts &substring_copy_numbers)
	{
		substring_copy_numbers.clear();
		
		std::size_t retval(1);
		std::size_t idx(1);
		std::size_t run_start_idx(0);
		std::size_t run_cn(1);
		for (auto const dd : divergence | ranges::view::drop(1))
		{
			if (dd <= lb)
				++run_cn;
			else
			{
				auto const string_idx(permutation[run_start_idx]);
				substring_copy_numbers.emplace_back(string_idx, run_cn);
				run_start_idx = idx;
				run_cn = 1;
				++retval;
			}
			
			++idx;
		}
		
		auto const string_idx(permutation[run_start_idx]);
		substring_copy_numbers.emplace_back(string_idx, run_cn);
		return retval;
	}

	
	template <
		typename t_position,
		typename t_string_positions,
		typename t_divergence,
		typename t_unique_indices
	>
	void unique_substring_indices(
		t_position const lb,
		t_string_positions const &permutation,
		t_divergence const &divergences,
		t_unique_indices &output_indices
	)
	{
		auto current_run_idx(permutation.front());
		std::size_t run_start(0);
		std::size_t run_end(0); // [ )
		for (auto const &pair : ranges::view::zip(permutation, divergences) | ranges::view::drop(1))
		{
			++run_end;
			auto const idx(std::get <0>(pair));
			auto const divergence(std::get <1>(pair));
			if (lb < divergence)
			{
				std::fill(output_indices.begin() + run_start, output_indices.end() + run_end, current_run_idx);
				run_start = run_end;
				current_run_idx = idx;
			}
			else
			{
				current_run_idx = std::min(idx, current_run_idx);
			}
		}
		
		std::fill(output_indices.begin() + run_start, output_indices.end() + run_end, current_run_idx);
	}
}

#endif

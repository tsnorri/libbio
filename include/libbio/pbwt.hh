/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_PBWT_HH
#define LIBBIO_PBWT_HH

#include <algorithm>
#include <cstddef>
#include <libbio/array_list.hh>
#include <numeric>


namespace libbio { namespace pbwt {
	
	// Build prefix and divergence arrays for PBWT.
	template <
		typename t_input_vector,
		typename t_alphabet,
		typename t_string_index_vector,
		typename t_character_index_vector,
		typename t_ci_rmq,
		typename t_count_vector
	>
	void build_prefix_and_divergence_arrays(
		t_input_vector const &inputs,						// Vector of input vectors.
		std::size_t const column_idx,						// Current column index, k.
		t_alphabet const &alphabet,							// Object for converting the input characters to consecutive integers.
		t_string_index_vector const &input_permutation,		// Order in which the input is iterated, a_k.
		t_character_index_vector const &input_divergence,	// Input divergence, d_k.
		t_ci_rmq const &input_divergence_rmq,				// Range maximum query support for d_k.
		t_string_index_vector &sorted_permutation,			// Sorted indexes, a_{k + 1}.
		t_character_index_vector &output_divergence,		// Output divergence, d_{k + 1}.
		t_count_vector &counts,								// Character counts.
		t_string_index_vector &previous_positions			// Previous position of each character.
	)
	{
		auto const input_count(inputs.size());
		auto const sigma(alphabet.sigma());
		
		// Sanity checks.
		assert(input_count == input_permutation.size());
		assert(input_count == input_divergence.size());
		assert(input_count == input_divergence_rmq.size());
		assert(sigma <= previous_positions.size());
		assert(sigma <= counts.size());
		
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
		std::fill(previous_positions.begin(), 1 + sigma + previous_positions.begin(), 0);
		{
			std::size_t i(0);
			for (auto const vec_idx : input_permutation)		// j in Algorithm 2.1.
			{
				auto const &vec(inputs[vec_idx]);
				auto const c(vec[column_idx]);
				auto const comp(alphabet.char_to_comp(c));
				auto const dst_idx(counts[comp]++);				// j' in Algorithm 2.1.
				sorted_permutation[dst_idx] = vec_idx;			// Store the identifier of the current string to a_{k + 1}.
			
				// Next value for d_{k + 1}.
				auto const prev_idx(previous_positions[comp]);	// i in Algorithm 2.1.
				assert(prev_idx <= i);
				if (0 == prev_idx)
					output_divergence[dst_idx] = 1 + column_idx;
				else
				{
					auto const max_div_idx(input_divergence_rmq(prev_idx ?: 1, i));
					auto const max_div(input_divergence[max_div_idx]);
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
			assert(output_val < count_list.size());
			
			// Add the new value. If the value did not occur in the list before,
			// it comes from the current column.
			auto const last_idx_1(count_list.last_index_1());
			if (last_idx_1 <= output_val)
			{
				assert(0 == count_list[output_val]);
				count_list.link(1, output_val, last_idx_1 - 1);
			}
			else
			{
				auto &count(count_list[output_val]);
				assert(count);
				++count;
			}
		}
		
		for (auto const input_val : input_divergence)
		{
			assert(input_val < count_list.size());
			
			// Decrement the old value and remove if necessary.
			auto input_it(count_list.find(input_val));
			assert(count_list.end() != input_it);
			auto &count(*input_it);
			assert(0 < count);
			if (0 == --count)
				count_list.erase(input_it, false);
		}
	}

	
#	define LIBBIO_PBWT_CONTEXT_TEMPLATE_DECL \
	template < \
		typename t_string_index_vector, \
		typename t_character_index_vector, \
		typename t_ci_rmq, \
		typename t_count_vector, \
		typename t_sequence_vector, \
		typename t_alphabet_type, \
		typename t_divergence_count_type \
	>
#	define LIBBIO_PBWT_CONTEXT_CLASS_DECL pbwt_context < \
		t_string_index_vector, \
		t_character_index_vector, \
		t_ci_rmq, \
		t_count_vector, \
		t_sequence_vector, \
		t_alphabet_type, \
		t_divergence_count_type \
	>
	
	LIBBIO_PBWT_CONTEXT_TEMPLATE_DECL
	class pbwt_context
	{
	protected:
		typedef t_string_index_vector					string_index_vector;
		typedef t_character_index_vector				character_index_vector;
		typedef t_ci_rmq								character_index_vector_rmq;
		typedef t_count_vector							count_vector;
		typedef t_sequence_vector						sequence_vector;
		typedef t_alphabet_type							alphabet_type;
		typedef array_list <t_divergence_count_type>	count_list;
		
	protected:
		sequence_vector const							*m_sequences{nullptr};
		alphabet_type const								*m_alphabet{nullptr};
		
		string_index_vector								m_input_permutation;
		string_index_vector								m_output_permutation;
		string_index_vector								m_inverse_input_permutation;
		character_index_vector							m_input_divergence;
		character_index_vector							m_output_divergence;
		count_vector									m_character_counts;
		string_index_vector								m_previous_positions;
		count_list										m_divergence_value_counts;
		
	public:
		pbwt_context(
			sequence_vector const &sequences,
			alphabet_type const &alphabet
		):
			m_sequences(&sequences),
			m_alphabet(&alphabet),
			m_input_permutation(m_sequences->size(), 0),
			m_output_permutation(m_sequences->size(), 0),
			m_inverse_input_permutation(m_sequences->size(), 0),
			m_input_divergence(m_sequences->size(), 0),
			m_output_divergence(m_sequences->size(), 0),
			m_character_counts(m_alphabet->sigma(), 0),
			m_previous_positions(m_alphabet->sigma(), 0)
		{
		}
		
		string_index_vector const &input_permutation() const { return m_input_permutation; }
		string_index_vector const &output_permutation() const { return m_output_permutation; }
		string_index_vector const &inverse_input_permutation() const { return m_inverse_input_permutation; }
		character_index_vector const &input_divergence() const { return m_input_divergence; }
		character_index_vector const &output_divergence() const { return m_output_divergence; }
		count_vector const &character_counts() const { return m_character_counts; }
		string_index_vector const &previous_positions() const { return m_previous_positions; }
		count_list const &divergence_value_counts() const { return m_divergence_value_counts; }
		std::size_t const size() const { return m_sequences->size(); }
		
		void prepare();
		void build_prefix_and_divergence_arrays(std::size_t const col);
		void update_inverse_input_permutation();
		void update_divergence_value_counts();
		void swap_input_and_output();
		
		// For debugging.
		void print_vectors() const;
		
	protected:
		template <typename t_vector>
		void print_vector(char const *name, t_vector const &vec) const;
		
		template <typename t_value>
		void print_count_list(char const *name, array_list <t_value> const &list) const;
	};
	
	
	LIBBIO_PBWT_CONTEXT_TEMPLATE_DECL
	void LIBBIO_PBWT_CONTEXT_CLASS_DECL::prepare()
	{
		// Fill the initial permutation and divergence.
		std::iota(m_input_permutation.begin(), m_input_permutation.end(), 0);
		std::fill(m_input_divergence.begin(), m_input_divergence.end(), 0);
		
		m_divergence_value_counts.resize(1 + m_sequences->front().size());
		m_divergence_value_counts.link(m_sequences->size(), 0);
		m_divergence_value_counts.set_first_element(0);
		m_divergence_value_counts.set_last_element(0);
	}
	
	
	LIBBIO_PBWT_CONTEXT_TEMPLATE_DECL
	void LIBBIO_PBWT_CONTEXT_CLASS_DECL::build_prefix_and_divergence_arrays(std::size_t const idx)
	{
		// Prepare input_divergence for Range Maximum Queries.
		character_index_vector_rmq input_divergence_rmq(&m_input_divergence);
		
		pbwt::build_prefix_and_divergence_arrays(
			*m_sequences,
			idx,
			*m_alphabet,
			m_input_permutation,
			m_input_divergence,
			input_divergence_rmq,
			m_output_permutation,
			m_output_divergence,
			m_character_counts,
			m_previous_positions
		);
	}
	
	
	LIBBIO_PBWT_CONTEXT_TEMPLATE_DECL
	void LIBBIO_PBWT_CONTEXT_CLASS_DECL::update_inverse_input_permutation()
	{
		pbwt::update_inverse_input_permutation(
			m_input_permutation,
			m_inverse_input_permutation
		);
	}
	
	
	LIBBIO_PBWT_CONTEXT_TEMPLATE_DECL
	void LIBBIO_PBWT_CONTEXT_CLASS_DECL::update_divergence_value_counts()
	{
		pbwt::update_divergence_value_counts(
			m_input_divergence,
			m_output_divergence,
			m_divergence_value_counts
		);
	}
	
	
	LIBBIO_PBWT_CONTEXT_TEMPLATE_DECL
	void LIBBIO_PBWT_CONTEXT_CLASS_DECL::swap_input_and_output()
	{
		using std::swap;
		swap(m_input_permutation, m_output_permutation);
		swap(m_input_divergence, m_output_divergence);
	}
	
	
	LIBBIO_PBWT_CONTEXT_TEMPLATE_DECL
	template <typename t_vector>
	void LIBBIO_PBWT_CONTEXT_CLASS_DECL::print_vector(char const *name, t_vector const &vec) const
	{
		std::cerr << name << ":";
		for (auto const val : vec)
			std::cerr << ' ' << +val;
		std::cerr << "\n";
	}
	
	
	LIBBIO_PBWT_CONTEXT_TEMPLATE_DECL
	template <typename t_value>
	void LIBBIO_PBWT_CONTEXT_CLASS_DECL::print_count_list(char const *name, array_list <t_value> const &list) const
	{
		std::cerr << name << ":";
		for (auto const &kv : list.const_pair_iterator_proxy())
			std::cerr << ' ' << kv.first << ": " << kv.second;
		std::cerr << "\n";
	}
	
	
	LIBBIO_PBWT_CONTEXT_TEMPLATE_DECL
	void LIBBIO_PBWT_CONTEXT_CLASS_DECL::print_vectors() const
	{
		std::cerr << "\n*** Current state\n";
		print_vector("input_permutation", m_input_permutation);
		print_vector("inverse_input_permutation", m_inverse_input_permutation);
		print_vector("output_permutation", m_output_permutation);
		print_vector("input_divergence", m_input_divergence);
		print_vector("output_divergence", m_output_divergence);
		print_vector("character_counts", m_character_counts);
		print_vector("previous_positions", m_previous_positions);
		print_count_list("divergence_value_counts", m_divergence_value_counts);
		std::cerr << std::flush;
	}
}}

#endif

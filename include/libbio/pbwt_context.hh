/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_PBWT_CONTEXT_HH
#define LIBBIO_PBWT_CONTEXT_HH

#include <libbio/matrix.hh>
#include <libbio/pbwt.hh>
#include <numeric>


namespace libbio { namespace pbwt {
	
#	define LIBBIO_PBWT_CONTEXT_TEMPLATE_DECL \
	template < \
		typename t_sequence_vector, \
		typename t_alphabet_type, \
		typename t_ci_rmq, \
		typename t_string_index_vector, \
		typename t_character_index_vector, \
		typename t_count_vector, \
		typename t_divergence_count_type \
	>
#	define LIBBIO_PBWT_CONTEXT_CLASS_DECL pbwt_context < \
		t_sequence_vector, \
		t_alphabet_type, \
		t_ci_rmq, \
		t_string_index_vector, \
		t_character_index_vector, \
		t_count_vector, \
		t_divergence_count_type \
	>
				
#	define LIBBIO_PBWT_BUFFERING_PBWT_CONTEXT_TEMPLATE_DECL \
	template < \
		typename t_sequence_vector, \
		typename t_alphabet_type, \
		typename t_ci_rmq, \
		typename t_string_index, \
		typename t_character_index, \
		typename t_character_count, \
		typename t_divergence_count, \
		template <typename> typename t_vector_tpl \
	>
#	define LIBBIO_PBWT_BUFFERING_PBWT_CONTEXT_CLASS_DECL buffering_pbwt_context < \
		t_sequence_vector, \
		t_alphabet_type, \
		t_ci_rmq, \
		t_string_index, \
		t_character_index, \
		t_character_count, \
		t_divergence_count, \
		t_vector_tpl \
	>
	
	// Forward declaration.
	LIBBIO_PBWT_BUFFERING_PBWT_CONTEXT_TEMPLATE_DECL
	class buffering_pbwt_context;
	
	
	LIBBIO_PBWT_CONTEXT_TEMPLATE_DECL
	class pbwt_context
	{
		template <
			typename,
			typename,
			typename,
			typename,
			typename,
			typename,
			typename,
			template <typename> typename
		>
		friend class buffering_pbwt_context;
		
	public:
		typedef t_string_index_vector					string_index_vector;
		typedef t_character_index_vector				character_index_vector;
		typedef t_ci_rmq								character_index_vector_rmq;
		typedef t_count_vector							count_vector;
		typedef t_sequence_vector						sequence_vector;
		typedef t_alphabet_type							alphabet_type;
		typedef array_list <t_divergence_count_type>	divergence_count_list;
		
	protected:
		sequence_vector const							*m_sequences{};
		alphabet_type const								*m_alphabet{};
		
		string_index_vector								m_input_permutation;
		string_index_vector								m_output_permutation;
		string_index_vector								m_inverse_input_permutation;
		character_index_vector							m_input_divergence;
		character_index_vector							m_output_divergence;
		count_vector									m_character_counts;
		string_index_vector								m_previous_positions;
		divergence_count_list							m_divergence_value_counts;
		
	public:
		pbwt_context() = default;
		pbwt_context(pbwt_context const &) = default;
		
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
		divergence_count_list const &divergence_value_counts() const { return m_divergence_value_counts; }
		std::size_t size() const { return m_sequences->size(); }
		
		void prepare();
		void build_prefix_and_divergence_arrays(std::size_t const col);
		void update_inverse_input_permutation();
		void update_divergence_value_counts();
		void swap_input_and_output();
		void clear();
		
		std::size_t unique_substring_count_lhs(std::size_t const lb) const { return libbio::pbwt::unique_substring_count(lb, m_input_divergence); }
		std::size_t unique_substring_count_rhs(std::size_t const lb) const { return libbio::pbwt::unique_substring_count(lb, m_output_divergence); }
		
		template <typename t_counts>
		std::size_t unique_substring_count_lhs(std::size_t const lb, t_counts &counts) const { return libbio::pbwt::unique_substring_count(lb, m_input_permutation, m_input_divergence, counts); }
		
		template <typename t_counts>
		std::size_t unique_substring_count_rhs(std::size_t const lb, t_counts &counts) const { return libbio::pbwt::unique_substring_count(lb, m_output_permutation, m_output_divergence, counts); }
		
		void unique_substring_indices_lhs(std::size_t const lb, string_index_vector &output_indices) const { return unique_substring_indices(lb, m_input_permutation, m_input_divergence, output_indices); }
		void unique_substring_indices_rhs(std::size_t const lb, string_index_vector &output_indices) const { return unique_substring_indices(lb, m_output_permutation, m_output_divergence, output_indices); }
		
		// For debugging.
		void print_vectors() const;
		
	protected:
		template <typename t_vector>
		void print_vector(char const *name, t_vector const &vec) const;
	};
	
	
	
	// Buffer for multiple PBWT arrays. Intended to be used in a producer thread.
	LIBBIO_PBWT_BUFFERING_PBWT_CONTEXT_TEMPLATE_DECL
	class buffering_pbwt_context
	{
	protected:
		typedef detail::array_list_item <t_divergence_count>	divergence_count_item;
		
	public:
		typedef t_sequence_vector								sequence_vector;
		typedef t_alphabet_type									alphabet_type;
		typedef t_ci_rmq										character_index_vector_rmq;
		typedef matrix <t_string_index>							string_index_matrix;
		typedef matrix <t_character_index>						character_index_matrix;
		typedef array_list <t_divergence_count>					divergence_count_list;
		typedef std::vector <divergence_count_list>				divergence_count_matrix;
		typedef t_vector_tpl <t_character_count>				count_vector;
		typedef t_vector_tpl <t_string_index>					string_index_vector;
		
		typedef pbwt_context <
			sequence_vector,
			alphabet_type,
			character_index_vector_rmq,
			t_vector_tpl <t_string_index>,
			t_vector_tpl <t_character_index>,
			count_vector,
			t_divergence_count
		> sample_context_type;
		
		struct sample_type
		{
			typedef sample_context_type context_type;
			
			context_type	context;
			t_string_index	index{};
			
			sample_type() = default;
			sample_type(t_string_index const index_):
				index(index_)
			{
			}
		};
		
		typedef std::vector <sample_type> sample_vector;
	
	protected:
		sequence_vector const									*m_sequences{};
		alphabet_type const										*m_alphabet{};
		std::uint64_t											m_sample_rate{std::numeric_limits <std::uint64_t>::max()};
		std::size_t												m_buffer_count{};
		std::size_t												m_sequence_size{};
		std::size_t												m_sequence_idx{};
		
		string_index_matrix										m_permutations;
		character_index_matrix									m_divergences;
		divergence_count_matrix									m_divergence_value_counts;
		count_vector											m_character_counts;
		string_index_vector										m_previous_positions;
		sample_vector											m_samples;
	
	public:
		buffering_pbwt_context() = default;
	
		buffering_pbwt_context(
			std::size_t const buffer_count,
			sequence_vector const &sequences,
			alphabet_type const &alphabet
		):
			m_sequences(&sequences),
			m_alphabet(&alphabet),
			m_buffer_count(buffer_count),
			m_sequence_size(m_sequences->front().size()),
			m_permutations(m_sequences->size(), m_buffer_count, 0),
			m_divergences(m_sequences->size(), m_buffer_count, 0),
			m_divergence_value_counts(m_buffer_count),
			m_character_counts(m_alphabet->sigma(), 0),
			m_previous_positions(m_alphabet->sigma(), 0)
		{
			for (auto &list : m_divergence_value_counts)
				list.resize(1 + m_sequence_size);
		}
		
		std::size_t sequence_idx() const { return m_sequence_idx; }
		std::size_t const size() const { return m_sequences->size(); }
		std::size_t const sequence_size() const { return m_sequences->front().size(); }
		divergence_count_list const &last_divergence_value_counts() const;
		sample_vector &samples() { return m_samples; }
		sample_vector const &samples() const { return m_samples; }
		
		
		std::size_t src_buffer_idx() const { return (m_buffer_count + m_sequence_idx - 1) % m_buffer_count; }
		std::size_t dst_buffer_idx() const { return m_sequence_idx % m_buffer_count; }
		
		void set_sample_rate(std::uint64_t const rate);
	
		void prepare();
	
		template <typename t_callback_fn, typename t_continue_fn>
		void process(
			t_callback_fn callback_fn,
			t_continue_fn continue_fn
		) { process(SIZE_MAX, callback_fn, continue_fn); }
			
		template <typename t_callback_fn, typename t_continue_fn>
		void process(
			std::size_t const caller_limit,
			t_callback_fn callback_fn,
			t_continue_fn continue_fn
		);
		
	protected:
		void copy_sample(sample_context_type &dst);
	};
	
	
	LIBBIO_PBWT_CONTEXT_TEMPLATE_DECL
	void LIBBIO_PBWT_CONTEXT_CLASS_DECL::prepare()
	{
		// Fill the initial permutation and divergence.
		std::iota(m_input_permutation.begin(), m_input_permutation.end(), 0);
		std::fill(m_input_divergence.begin(), m_input_divergence.end(), 0);
		
		m_divergence_value_counts.resize(1 + m_sequences->front().size());
		m_divergence_value_counts.link(m_sequences->size(), 0);
		m_divergence_value_counts.set_last_element(0);
	}
	
	
	LIBBIO_PBWT_CONTEXT_TEMPLATE_DECL
	void LIBBIO_PBWT_CONTEXT_CLASS_DECL::clear()
	{
		m_sequences = nullptr;
		m_alphabet = nullptr;
		
		m_input_permutation.clear();
		m_output_permutation.clear();
		m_inverse_input_permutation.clear();
		m_input_divergence.clear();
		m_output_divergence.clear();
		m_character_counts.clear();
		m_previous_positions.clear();
		m_divergence_value_counts.clear();
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
	void LIBBIO_PBWT_CONTEXT_CLASS_DECL::print_vectors() const
	{
		std::cerr << "\n*** Current state\n";
		print_vector("input_permutation", m_input_permutation);
		print_vector("inverse_input_permutation", m_inverse_input_permutation);
		print_vector("output_permutation", m_output_permutation);
		print_vector("input_divergence", m_input_divergence);
		print_vector("output_divergence", m_output_divergence);
		std::cerr << "divergence_value_counts: " << m_divergence_value_counts << '\n';
		print_vector("character_counts", m_character_counts);
		print_vector("previous_positions", m_previous_positions);
		std::cerr << std::flush;
	}
	
	LIBBIO_PBWT_BUFFERING_PBWT_CONTEXT_TEMPLATE_DECL
	auto LIBBIO_PBWT_BUFFERING_PBWT_CONTEXT_CLASS_DECL::last_divergence_value_counts() const -> divergence_count_list const &
	{
		auto const idx((m_sequence_idx - 1) % m_buffer_count);
		assert(idx < m_divergence_value_counts.size());
		return m_divergence_value_counts[idx];
	}
	
	
	LIBBIO_PBWT_BUFFERING_PBWT_CONTEXT_TEMPLATE_DECL
	void LIBBIO_PBWT_BUFFERING_PBWT_CONTEXT_CLASS_DECL::prepare()
	{
		auto const last(m_buffer_count - 1);
		auto input_permutation(m_permutations.column(last));
		auto input_divergence(m_divergences.column(last));
		auto &divergence_value_counts(m_divergence_value_counts[last]);
		
		// Fill the initial permutation and divergence.
		std::iota(input_permutation.begin(), input_permutation.end(), 0);
		std::fill(input_divergence.begin(), input_divergence.end(), 0);
		
		divergence_value_counts.link(m_sequences->size(), 0);
		divergence_value_counts.set_last_element(0);
	}
	
	
	LIBBIO_PBWT_BUFFERING_PBWT_CONTEXT_TEMPLATE_DECL
	template <typename t_callback_fn, typename t_continue_fn>
	void LIBBIO_PBWT_BUFFERING_PBWT_CONTEXT_CLASS_DECL::process(
		std::size_t const caller_limit,
		t_callback_fn callback_fn,
		t_continue_fn continue_fn
	)
	{
		// First src is the rightmost vector.
		std::size_t const limit(std::min({caller_limit, m_sequence_idx + m_buffer_count, m_sequence_size}));
		auto src_idx(src_buffer_idx());
		auto dst_idx(dst_buffer_idx());
		
		while (m_sequence_idx < limit)
		{
			assert(src_idx < m_buffer_count);
			assert(dst_idx < m_buffer_count);
			
			auto input_permutation(m_permutations.column(src_idx));
			auto output_permutation(m_permutations.column(dst_idx));
			auto input_divergence(m_divergences.column(src_idx));
			auto output_divergence(m_divergences.column(dst_idx));
			auto const &input_divergence_value_counts(m_divergence_value_counts[src_idx]);
			auto &output_divergence_value_counts(m_divergence_value_counts[dst_idx]);
			
			// Prepare input_divergence for Range Maximum Queries.
			character_index_vector_rmq input_divergence_rmq(&input_divergence);
			
			pbwt::build_prefix_and_divergence_arrays(
				*m_sequences,
				m_sequence_idx,
				*m_alphabet,
				input_permutation,
				input_divergence,
				input_divergence_rmq,
				output_permutation,
				output_divergence,
				m_character_counts,
				m_previous_positions
			);
			
			// Copy the previous values and update.
			output_divergence_value_counts = input_divergence_value_counts;
			pbwt::update_divergence_value_counts(
				input_divergence,
				output_divergence,
				output_divergence_value_counts
			);
			
			callback_fn(m_sequence_idx, output_divergence_value_counts);
			
			if (0 == m_sequence_idx % m_sample_rate)
			{
				auto &sample(m_samples.emplace_back(m_sequence_idx));
				copy_sample(sample.context);
				sample.context.swap_input_and_output();
			}
			
			src_idx = dst_idx;
			++dst_idx;
			dst_idx %= m_buffer_count;
			++m_sequence_idx;
		}
		
		continue_fn();
	}
	
	
	LIBBIO_PBWT_BUFFERING_PBWT_CONTEXT_TEMPLATE_DECL
	void LIBBIO_PBWT_BUFFERING_PBWT_CONTEXT_CLASS_DECL::set_sample_rate(std::uint64_t const sample_rate)
	{
		libbio_always_assert(0 == (sample_rate & (sample_rate - 1)), "Expected sample rate to be a power of two.");
		m_sample_rate = sample_rate;
	}
	
	
	LIBBIO_PBWT_BUFFERING_PBWT_CONTEXT_TEMPLATE_DECL
	void LIBBIO_PBWT_BUFFERING_PBWT_CONTEXT_CLASS_DECL::copy_sample(sample_context_type &dst)
	{
		dst.m_sequences = m_sequences;
		dst.m_alphabet = m_alphabet;
		
		auto const src_idx(src_buffer_idx());
		auto const dst_idx(dst_buffer_idx());
		
		auto const &input_permutation(m_permutations.column(src_idx));
		auto const &output_permutation(m_permutations.column(dst_idx));
		auto const &input_divergence(m_divergences.column(src_idx));
		auto const &output_divergence(m_divergences.column(dst_idx));
		auto const &divergence_value_counts(m_divergence_value_counts[dst_idx]);
		
		resize_and_copy(input_permutation, dst.m_input_permutation);
		resize_and_copy(output_permutation, dst.m_output_permutation);
		
		dst.m_inverse_input_permutation.resize(input_permutation.size());
		std::fill(dst.m_inverse_input_permutation.begin(), dst.m_inverse_input_permutation.end(), 0);
		
		resize_and_copy(input_divergence, dst.m_input_divergence);
		resize_and_copy(output_divergence, dst.m_output_divergence);
		resize_and_copy(m_character_counts, dst.m_character_counts);
		resize_and_copy(m_previous_positions, dst.m_previous_positions);
		dst.m_divergence_value_counts = divergence_value_counts;
	}
}}

#endif

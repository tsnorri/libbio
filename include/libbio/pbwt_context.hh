/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_PBWT_CONTEXT_HH
#define LIBBIO_PBWT_CONTEXT_HH

#include <libbio/algorithm.hh>
#include <libbio/matrix.hh>
#include <libbio/pbwt.hh>
#include <numeric>
#include <ostream>


namespace libbio { namespace pbwt { namespace detail {
	
	template <typename t_sample_context_type>
	struct buffering_pbwt_context_sample final
	{
		typedef t_sample_context_type						context_type;
		typedef typename context_type::string_index_type	string_index;
		
		context_type	context;
		string_index	rb{};		// Right bound; equal to the 1-based PBWT array index.
		
		buffering_pbwt_context_sample() = default;
		buffering_pbwt_context_sample(string_index const rb_):
			rb(rb_)
		{
		}
	};
	
	
	template <bool t_use_semaphore>
	struct callback_function_helper
	{
		template <typename t_fn>
		static inline void call(t_fn &&fn, std::size_t sequence_idx, std::size_t buffer_idx, dispatch_semaphore_t process_sema)
		{
			fn(sequence_idx);
		}
	};
	
	template <>
	struct callback_function_helper <true>
	{
		template <typename t_fn>
		static inline void call(t_fn &&fn, std::size_t sequence_idx, std::size_t buffer_idx, dispatch_semaphore_t process_sema)
		{
			fn(sequence_idx, buffer_idx, process_sema);
		}
	};
}}}


namespace libbio { namespace pbwt {
	
	enum pbwt_context_field : std::uint8_t
	{
		NONE						= 0,
		INPUT_PERMUTATION			= 0x1,
		OUTPUT_PERMUTATION			= 0x2,
		INVERSE_INPUT_PERMUTATION	= 0x4,
		INPUT_DIVERGENCE			= 0x8,
		OUTPUT_DIVERGENCE			= 0x10,
		CHARACTER_COUNTS			= 0x20,
		PREVIOUS_POSITIONS			= 0x40,
		DIVERGENCE_VALUE_COUNTS		= 0x80,
		ALL							= 0xff
	};

#	define LIBBIO_PBWT_CONTEXT_TEMPLATE_ARGS \
	typename t_sequence_vector, \
	typename t_alphabet_type, \
	typename t_ci_rmq, \
	typename t_string_index_vector, \
	typename t_character_index_vector, \
	typename t_count_vector, \
	typename t_divergence_count_type
	
#	define LIBBIO_PBWT_CONTEXT_TEMPLATE_DECL \
	template < \
		LIBBIO_PBWT_CONTEXT_TEMPLATE_ARGS \
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
		typedef t_string_index_vector						string_index_vector;
		typedef typename string_index_vector::value_type	string_index_type;
		typedef t_character_index_vector					character_index_vector;
		typedef t_ci_rmq									character_index_vector_rmq;
		typedef t_count_vector								count_vector;
		typedef t_sequence_vector							sequence_vector;
		typedef t_alphabet_type								alphabet_type;
		typedef array_list <t_divergence_count_type>		divergence_count_list;
		
	protected:
		sequence_vector const								*m_sequences{};
		alphabet_type const									*m_alphabet{};
		
		string_index_vector									m_input_permutation;
		string_index_vector									m_output_permutation;
		string_index_vector									m_inverse_input_permutation;
		character_index_vector								m_input_divergence;
		character_index_vector								m_output_divergence;
		count_vector										m_character_counts;
		string_index_vector									m_previous_positions;
		divergence_count_list								m_divergence_value_counts;
		
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
			m_previous_positions(1 + m_alphabet->sigma(), 0)
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
		void clear(pbwt_context_field fields = pbwt_context_field::ALL);
		
		std::size_t unique_substring_count_lhs(std::size_t const lb) const { return libbio::pbwt::unique_substring_count(lb, m_input_divergence); }
		std::size_t unique_substring_count_rhs(std::size_t const lb) const { return libbio::pbwt::unique_substring_count(lb, m_output_divergence); }

		template <typename t_counts>
		std::size_t unique_substring_count_lhs(std::size_t const lb, t_counts &counts) const { return libbio::pbwt::unique_substring_count(lb, m_input_divergence, counts); }
		
		template <typename t_counts>
		std::size_t unique_substring_count_rhs(std::size_t const lb, t_counts &counts) const { return libbio::pbwt::unique_substring_count(lb, m_output_divergence, counts); }
		
		template <typename t_counts>
		std::size_t unique_substring_count_idxs_lhs(std::size_t const lb, t_counts &counts) const { return libbio::pbwt::unique_substring_count_idxs(lb, m_input_permutation, m_input_divergence, counts); }
		
		template <typename t_counts>
		std::size_t unique_substring_count_idxs_rhs(std::size_t const lb, t_counts &counts) const { return libbio::pbwt::unique_substring_count_idxs(lb, m_output_permutation, m_output_divergence, counts); }
		
		// For Boost.Serialization.
		template <typename t_archive>
		void boost_serialize(t_archive &ar, unsigned int version);
		
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
		typedef libbio::detail::array_list_item <t_divergence_count>	divergence_count_item;
		
	public:
		typedef t_sequence_vector										sequence_vector;
		typedef t_alphabet_type											alphabet_type;
		typedef t_ci_rmq												character_index_vector_rmq;
		typedef matrix <t_string_index>									string_index_matrix;
		typedef matrix <t_character_index>								character_index_matrix;
		typedef array_list <t_divergence_count>							divergence_count_list;
		typedef std::vector <divergence_count_list>						divergence_count_matrix;
		typedef t_vector_tpl <t_character_count>						count_vector;
		typedef t_vector_tpl <t_string_index>							string_index_vector;
		
		typedef pbwt_context <
			sequence_vector,
			alphabet_type,
			character_index_vector_rmq,
			t_vector_tpl <t_string_index>,
			t_vector_tpl <t_character_index>,
			count_vector,
			t_divergence_count
		> sample_context_type;
		
		typedef detail::buffering_pbwt_context_sample <sample_context_type>	sample_type;
		typedef std::vector <sample_type>									sample_vector;
	
	protected:
		sequence_vector const									*m_sequences{};
		alphabet_type const										*m_alphabet{};
		libbio::dispatch_ptr <dispatch_semaphore_t>				m_process_sema{};
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
			m_process_sema(dispatch_semaphore_create(buffer_count)),
			m_buffer_count(buffer_count),
			m_sequence_size(m_sequences->front().size()),
			m_permutations(m_sequences->size(), m_buffer_count, 0),
			m_divergences(m_sequences->size(), m_buffer_count, 0),
			m_divergence_value_counts(m_buffer_count),
			m_character_counts(m_alphabet->sigma(), 0),
			m_previous_positions(1 + m_alphabet->sigma(), 0)
		{
			for (auto &list : m_divergence_value_counts)
				list.resize(1 + m_sequence_size);
		}
		
		std::size_t sequence_idx() const { return m_sequence_idx; }
		std::size_t const size() const { return m_sequences->size(); }
		std::size_t const sequence_size() const { return m_sequences->front().size(); }
		std::size_t const buffer_count() const { return m_buffer_count; }
		divergence_count_list const &last_divergence_value_counts() const;
		sample_vector &samples() { return m_samples; }
		sample_vector const &samples() const { return m_samples; }
		
		typename string_index_matrix::const_slice_type const &permutation(std::size_t const idx) const { return m_permutations.column(idx); }
		typename character_index_matrix::const_slice_type const &divergence(std::size_t const idx) const { return m_divergences.column(idx); }
		divergence_count_list const &divergence_value_counts(std::size_t const idx) const { return m_divergence_value_counts[idx]; }
		
		std::size_t src_buffer_idx() const { return (m_buffer_count + m_sequence_idx - 1) % m_buffer_count; }
		std::size_t dst_buffer_idx() const { return m_sequence_idx % m_buffer_count; }
		
		void set_sample_rate(std::uint64_t const rate);
	
		void prepare();
		
		template <bool t_use_semaphore, typename t_callback_fn, typename t_continue_fn, typename t_dispatch_copy_sample_fn>
		void process(
			t_callback_fn callback_fn,
			t_continue_fn continue_fn,
			t_dispatch_copy_sample_fn dispatch_copy_sample_fn
		) { process <t_use_semaphore>(SIZE_MAX, callback_fn, continue_fn, dispatch_copy_sample_fn); }
			
		template <bool t_use_semaphore, typename t_callback_fn, typename t_continue_fn, typename t_dispatch_copy_sample_fn>
		void process(
			std::size_t const caller_limit,
			t_callback_fn callback_fn,
			t_continue_fn continue_fn,
			t_dispatch_copy_sample_fn dispatch_copy_sample_fn
		);
		
	protected:
		void copy_sample(std::size_t src_idx, std::size_t dst_idx, sample_context_type &dst) const;
	};
	
	
	template <typename t_sample_context_type>
	std::ostream &operator<<(std::ostream &os, detail::buffering_pbwt_context_sample <t_sample_context_type> const &sample)
	{
		os << sample.rb;
		return os;
	}
	
	
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
	void LIBBIO_PBWT_CONTEXT_CLASS_DECL::clear(pbwt_context_field fields)
	{
		m_sequences = nullptr;
		m_alphabet = nullptr;
		
		// FIXME: this is slightly problematic since the vectors need not be std::vectors.
		if (pbwt_context_field::INPUT_PERMUTATION & fields)
			clear_and_resize_vector(m_input_permutation);
		
		if (pbwt_context_field::OUTPUT_PERMUTATION & fields)
			clear_and_resize_vector(m_output_permutation);
		
		if (pbwt_context_field::INVERSE_INPUT_PERMUTATION & fields)
			clear_and_resize_vector(m_inverse_input_permutation);
		
		if (pbwt_context_field::INPUT_DIVERGENCE & fields)
			clear_and_resize_vector(m_input_divergence);
		
		if (pbwt_context_field::OUTPUT_DIVERGENCE & fields)
			clear_and_resize_vector(m_output_divergence);
		
		if (pbwt_context_field::CHARACTER_COUNTS & fields)
			clear_and_resize_vector(m_character_counts);
		
		if (pbwt_context_field::PREVIOUS_POSITIONS & fields)
			clear_and_resize_vector(m_previous_positions);
		
		if (pbwt_context_field::DIVERGENCE_VALUE_COUNTS & fields)
			m_divergence_value_counts.clear(true);
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
	
	
	LIBBIO_PBWT_CONTEXT_TEMPLATE_DECL
	template <typename t_archive>
	void LIBBIO_PBWT_CONTEXT_CLASS_DECL::boost_serialize(t_archive &ar, unsigned int version)
	{
		ar & m_input_permutation;
		ar & m_output_permutation;
		ar & m_inverse_input_permutation;
		ar & m_input_divergence;
		ar & m_output_divergence;
		ar & m_character_counts;
		ar & m_previous_positions;
		ar & m_divergence_value_counts;
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
	template <bool t_use_semaphore, typename t_callback_fn, typename t_continue_fn, typename t_dispatch_copy_sample_fn>
	void LIBBIO_PBWT_BUFFERING_PBWT_CONTEXT_CLASS_DECL::process(
		std::size_t const caller_limit,
		t_callback_fn callback_fn,
		t_continue_fn continue_fn,
		t_dispatch_copy_sample_fn dispatch_copy_sample_fn
	)
	{
		// The algorithm does not work if the number of the source buffer for copying a sample
		// cannot be stored for the duration of filling all the buffers in-between.
		assert(m_buffer_count <= m_sample_rate);
		
		m_samples.reserve(1 + m_sequence_size / m_sample_rate);
		
		// Create a semaphore and a counter for copying the samples.
		dispatch_ptr <dispatch_semaphore_t> copy_sample_sema(dispatch_semaphore_create(0));
		std::size_t copy_sample_src_idx(SIZE_MAX);
		
		// First src is the rightmost vector.
		std::size_t const limit(std::min({caller_limit, m_sequence_size}));
		auto src_idx(src_buffer_idx());
		auto dst_idx(dst_buffer_idx());
		
		while (m_sequence_idx < limit)
		{
			assert(src_idx < m_buffer_count);
			assert(dst_idx < m_buffer_count);
			
			// Wait until the consumer thread has processed the buffered arrays.
			if (t_use_semaphore)
				dispatch_semaphore_wait(*m_process_sema, DISPATCH_TIME_FOREVER);
			
			// Wait until the copying task is complete.
			if (copy_sample_src_idx == dst_idx)
			{
				dispatch_semaphore_wait(*copy_sample_sema, DISPATCH_TIME_FOREVER);
				copy_sample_src_idx = SIZE_MAX;
			}
			
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
			
			// Callback should take different arguments if m_process_sema is to be used.
			detail::callback_function_helper <t_use_semaphore>::call(callback_fn, m_sequence_idx, dst_idx, *m_process_sema);
			
			// Check if a sample needs to be copied.
			if (0 == m_sequence_idx % m_sample_rate)
			{
				// Remember the position.
				copy_sample_src_idx = src_idx;
				
				// Variables for the block.
				// Having a reference to an element of m_samples should be safe
				// b.c. the next copying task will wait in the beginning of the
				// while loop for dispatch_semaphore_signal below.
				auto &sample(m_samples.emplace_back(1 + m_sequence_idx));
				auto sema(*copy_sample_sema); // Get the stored pointer.
				
				dispatch_copy_sample_fn(^{
					// sample will be a reference.
					copy_sample(src_idx, dst_idx, sample.context);
					sample.context.swap_input_and_output();
					
					dispatch_semaphore_signal(sema);
				});
			}
			
			src_idx = dst_idx;
			++dst_idx;
			dst_idx %= m_buffer_count;
			++m_sequence_idx;
		}
		
		// Wait for copying if needed.
		if (SIZE_MAX != copy_sample_src_idx)
			dispatch_semaphore_wait(*copy_sample_sema, DISPATCH_TIME_FOREVER);
		
		continue_fn();
	}
	
	
	LIBBIO_PBWT_BUFFERING_PBWT_CONTEXT_TEMPLATE_DECL
	void LIBBIO_PBWT_BUFFERING_PBWT_CONTEXT_CLASS_DECL::set_sample_rate(std::uint64_t const sample_rate)
	{
		//libbio_always_assert(0 == (sample_rate & (sample_rate - 1)), "Expected sample rate to be a power of two.");
		m_sample_rate = max_ct(m_buffer_count, sample_rate);
	}
	
	
	LIBBIO_PBWT_BUFFERING_PBWT_CONTEXT_TEMPLATE_DECL
	void LIBBIO_PBWT_BUFFERING_PBWT_CONTEXT_CLASS_DECL::copy_sample(std::size_t const src_idx, std::size_t const dst_idx, sample_context_type &dst) const
	{
		dst.m_sequences = m_sequences;
		dst.m_alphabet = m_alphabet;
		
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


namespace boost { namespace serialization {
	
	template <typename t_archive, typename t_sample_context_type>
	void serialize(
		t_archive &ar,
		libbio::pbwt::detail::buffering_pbwt_context_sample <t_sample_context_type> &sample,
		unsigned int const version
	)
	{
		ar & sample.context;
		ar & sample.rb;
	}
	
	
	template <typename t_archive, LIBBIO_PBWT_CONTEXT_TEMPLATE_ARGS>
	void serialize(
		t_archive &ar,
		libbio::pbwt::LIBBIO_PBWT_CONTEXT_CLASS_DECL &context,
		unsigned int version
	)
	{
		context.boost_serialize(ar, version);
	}
}}


#endif

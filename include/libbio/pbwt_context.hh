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
	
	// Adapt RMQ support’s constructor.
	// Assume SDSL’s constructor parameters.
	template <typename t_rmq>
	class rmq_adapter
	{
	public:
		typedef typename t_rmq::size_type	size_type;
		
	protected:
		t_rmq	m_rmq;
		
	public:
		rmq_adapter() = default;
		
		template <typename t_character_index_vector, typename t_divergence_vector>
		rmq_adapter(
			t_divergence_vector const &input_divergence,
			t_divergence_vector const &,
			t_character_index_vector const &
		):
			m_rmq(&input_divergence)
		{
		}
		
		size_type operator()(size_type j, size_type i) const { return m_rmq(j, i); }
		size_type size() const { return m_rmq.size(); }
	};
	
	// Specialization for libbio::pbwt::dynamic_pbwt_rmq, which uses the previous arrays.
	template <typename t_character_index_vector, typename t_divergence_vector>
	class rmq_adapter <::libbio::pbwt::dynamic_pbwt_rmq <t_character_index_vector, t_divergence_vector>>
	{
	public:
		typedef ::libbio::pbwt::dynamic_pbwt_rmq <t_character_index_vector, t_divergence_vector>	rmq_type;
		typedef typename rmq_type::size_type														size_type;
		
	protected:
		rmq_type m_rmq;
		
	public:
		rmq_adapter() = default;
		
		rmq_adapter(
			t_divergence_vector const &,
			t_divergence_vector &prev_divergence,
			t_character_index_vector &prev_permutation
		):
			m_rmq(prev_divergence, prev_permutation)
		{
		}
		
		size_type operator()(size_type j, size_type i) { return m_rmq(j, i); }	// Not const.
		size_type size() const { return m_rmq.siz(); }
	};
}}}


namespace libbio { namespace pbwt {
	
	enum context_field : std::uint8_t
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
	
	
	LIBBIO_PBWT_CONTEXT_TEMPLATE_DECL
	class pbwt_context
	{
	public:
		typedef t_string_index_vector						string_index_vector;
		typedef typename string_index_vector::value_type	string_index_type;
		typedef t_character_index_vector					character_index_vector;
		typedef t_ci_rmq									character_index_vector_rmq;
		typedef t_count_vector								count_vector;
		typedef t_sequence_vector							sequence_vector;
		typedef t_alphabet_type								alphabet_type;
		typedef array_list <t_divergence_count_type>		divergence_count_list;
		typedef pbwt_context								sample_context_type;
		typedef std::vector <pbwt_context>					sample_vector;
		
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
		sample_vector										m_samples;
		
		std::uint64_t										m_sample_rate{std::numeric_limits <std::uint64_t>::max()};
		std::size_t											m_sequence_idx{};
		context_field										m_fields_in_use{
																context_field::ALL &
																~context_field::INVERSE_INPUT_PERMUTATION &
																~context_field::DIVERGENCE_VALUE_COUNTS
															};
		
	public:
		pbwt_context() = default;
		pbwt_context(pbwt_context const &) = default;
		
		pbwt_context(
			sequence_vector const &sequences,
			alphabet_type const &alphabet,
			context_field const extra_fields = context_field::NONE
		):
			m_sequences(&sequences),
			m_alphabet(&alphabet),
			m_input_permutation(m_sequences->size(), 0),
			m_output_permutation(m_sequences->size(), 0),
			m_inverse_input_permutation(extra_fields & context_field::INVERSE_INPUT_PERMUTATION ? m_sequences->size() : 0, 0),
			m_input_divergence(m_sequences->size(), 0),
			m_output_divergence(m_sequences->size(), 0),
			m_character_counts(m_alphabet->sigma(), 0),
			m_previous_positions(1 + m_alphabet->sigma(), 0)
		{
			m_fields_in_use = static_cast <context_field const>(m_fields_in_use | extra_fields);
		}
		
		string_index_vector const &input_permutation() const { return m_input_permutation; }
		string_index_vector const &output_permutation() const { return m_output_permutation; }
		string_index_vector const &inverse_input_permutation() const { return m_inverse_input_permutation; }
		character_index_vector const &input_divergence() const { return m_input_divergence; }
		character_index_vector const &output_divergence() const { return m_output_divergence; }
		count_vector const &character_counts() const { return m_character_counts; }
		string_index_vector const &previous_positions() const { return m_previous_positions; }
		
		// FIXME: remove some of these.
		divergence_count_list const &divergence_value_counts() const { return m_divergence_value_counts; }
		divergence_count_list const &last_divergence_value_counts() const { return m_divergence_value_counts; }
		divergence_count_list const &output_divergence_value_counts() const { return m_divergence_value_counts; }
		
		std::size_t sequence_length() const { return m_sequences->front().size(); }
		std::size_t size() const { return m_sequences->size(); }
		std::size_t sequence_idx() const { return m_sequence_idx; } // Right bound.
		sample_vector &samples() { return m_samples; }
		sample_vector const &samples() const { return m_samples; }
		void set_sample_rate(std::uint64_t const sample_rate) { m_sample_rate = sample_rate; }
		void set_fields_in_use(context_field fields) { m_fields_in_use = fields; }
		
		void prepare();
		void build_prefix_and_divergence_arrays();
		void update_inverse_input_permutation();
		void update_divergence_value_counts();
		void swap_input_and_output();
		void copy_fields_in_use(pbwt_context const &other);
		void clear_unused_fields();
		
		template <
			context_field t_extra_fields = context_field::NONE,
			typename t_callback_fn
		>
		void process(
			t_callback_fn &&callback_fn
		) { process <t_extra_fields>(SIZE_MAX, callback_fn); }
		
		template <
			context_field t_extra_fields = context_field::NONE,
			typename t_callback_fn
		>
		void process(
			std::size_t const caller_limit,
			t_callback_fn &&callback_fn
		);
		
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
	void LIBBIO_PBWT_CONTEXT_CLASS_DECL::clear_unused_fields()
	{
		m_sequences = nullptr;
		m_alphabet = nullptr;
		
		if (! (context_field::INPUT_PERMUTATION & m_fields_in_use))
			clear_and_resize_vector(m_input_permutation);
		
		if (! (context_field::OUTPUT_PERMUTATION & m_fields_in_use))
			clear_and_resize_vector(m_output_permutation);
		
		if (! (context_field::INVERSE_INPUT_PERMUTATION & m_fields_in_use))
			clear_and_resize_vector(m_inverse_input_permutation);
		
		if (! (context_field::INPUT_DIVERGENCE & m_fields_in_use))
			clear_and_resize_vector(m_input_divergence);
		
		if (! (context_field::OUTPUT_DIVERGENCE & m_fields_in_use))
			clear_and_resize_vector(m_output_divergence);
		
		if (! (context_field::CHARACTER_COUNTS & m_fields_in_use))
			clear_and_resize_vector(m_character_counts);
		
		if (! (context_field::PREVIOUS_POSITIONS & m_fields_in_use))
			clear_and_resize_vector(m_previous_positions);
		
		if (! (context_field::DIVERGENCE_VALUE_COUNTS & m_fields_in_use))
			m_divergence_value_counts.clear(true);
	}
	
	
	LIBBIO_PBWT_CONTEXT_TEMPLATE_DECL
	void LIBBIO_PBWT_CONTEXT_CLASS_DECL::copy_fields_in_use(pbwt_context const &other)
	{
		m_sequences = other.m_sequences;
		m_alphabet = other.m_alphabet;
		m_sequence_idx = other.m_sequence_idx;
		
		if (context_field::INPUT_PERMUTATION & m_fields_in_use)
			m_input_permutation = other.m_input_permutation;
		
		if (context_field::OUTPUT_PERMUTATION & m_fields_in_use)
			m_output_permutation = other.m_output_permutation;
		
		if (context_field::INVERSE_INPUT_PERMUTATION & m_fields_in_use)
			m_inverse_input_permutation = other.m_inverse_input_permutation;
		
		if (context_field::INPUT_DIVERGENCE & m_fields_in_use)
			m_input_divergence = other.m_input_divergence;
		
		if (context_field::OUTPUT_DIVERGENCE & m_fields_in_use)
			m_output_divergence = other.m_output_divergence;
		
		if (context_field::CHARACTER_COUNTS & m_fields_in_use)
			m_character_counts = other.m_character_counts;
		
		if (context_field::PREVIOUS_POSITIONS & m_fields_in_use)
			m_previous_positions = other.m_previous_positions;
		
		if (context_field::DIVERGENCE_VALUE_COUNTS & m_fields_in_use)
			m_divergence_value_counts = other.m_divergence_value_counts;
	}
	
	
	LIBBIO_PBWT_CONTEXT_TEMPLATE_DECL
	void LIBBIO_PBWT_CONTEXT_CLASS_DECL::build_prefix_and_divergence_arrays()
	{
		// Prepare input_divergence for Range Maximum Queries.
		detail::rmq_adapter <character_index_vector_rmq> input_divergence_rmq(
			m_input_divergence,
			m_output_divergence,
			m_output_permutation
		);
		
		pbwt::build_prefix_and_divergence_arrays(
			*m_sequences,
			m_sequence_idx,
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
		std::cerr << "sequence_idx: " << m_sequence_idx << '\n';
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
	
	
	LIBBIO_PBWT_CONTEXT_TEMPLATE_DECL
	template <
		context_field t_extra_fields,
		typename t_callback_fn
	>
	void LIBBIO_PBWT_CONTEXT_CLASS_DECL::process(
		std::size_t const caller_limit,
		t_callback_fn &&callback_fn
	)
	{
		auto const seq_length(sequence_length());
		m_samples.reserve(1 + seq_length / m_sample_rate);
		
		std::size_t const limit(std::min({caller_limit, seq_length}));
		while (m_sequence_idx < limit)
		{
			build_prefix_and_divergence_arrays();
			
			if (t_extra_fields & context_field::INVERSE_INPUT_PERMUTATION)
				update_inverse_input_permutation();
			
			if (t_extra_fields & context_field::DIVERGENCE_VALUE_COUNTS)
				update_divergence_value_counts();
			
			callback_fn();
			
			// Check if a sample needs to be copied.
			if (0 == m_sequence_idx % m_sample_rate)
			{
				auto &sample(m_samples.emplace_back());
				sample.set_fields_in_use(m_fields_in_use);
				sample.copy_fields_in_use(*this);
				++sample.m_sequence_idx;
				sample.swap_input_and_output();
			}
			
			swap_input_and_output();
			++m_sequence_idx;
		}
	}
}}


namespace boost { namespace serialization {
	
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

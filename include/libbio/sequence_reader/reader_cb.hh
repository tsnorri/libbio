/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_SEQUENCE_READER_READER_CB_HH
#define LIBBIO_SEQUENCE_READER_READER_CB_HH

#include <libbio/fasta_reader.hh>
#include <memory>
#include <vector>


namespace libbio { namespace sequence_reader {
	
	template <typename t_vector_source>
	class reader_cb
	{
	protected:
		typedef std::vector <std::vector <std::uint8_t>>	sequence_vector;
		
	protected:
		sequence_vector	*m_sequences{nullptr};
		
	public:
		reader_cb(sequence_vector &vec):
			m_sequences(&vec)
		{
		}
		
		void handle_sequence(
			std::unique_ptr <typename t_vector_source::vector_type> &seq_ptr,
			std::size_t const &seq_length,
			t_vector_source &vector_source
		)
		{
			auto &seq(m_sequences->emplace_back(*seq_ptr));
			seq.resize(seq_length);
			seq_ptr.reset();
			vector_source.set_vector_length(seq_length);
			// Don't return the vector to the source.
		}
		
		void start() {}
		void finish() {}
	};
	
	
	template <typename t_vector_source>
	class fasta_reader_cb final : public reader_cb <t_vector_source>
	{
	public:
		using reader_cb <t_vector_source>::reader_cb;
		
		void handle_sequence(
			std::string const &identifier,
			std::unique_ptr <typename t_vector_source::vector_type> &seq_ptr,
			std::size_t const &seq_length,
			t_vector_source &vector_source
		)
		{
			reader_cb <t_vector_source>::handle_sequence(seq_ptr, seq_length, vector_source);
		}
	};
	
	
	template <typename t_vector_source>
	class line_reader_cb final : public reader_cb <t_vector_source>
	{
	public:
		using reader_cb <t_vector_source>::reader_cb;
		
		void handle_sequence(
			uint32_t line,
			std::unique_ptr <typename t_vector_source::vector_type> &seq_ptr,
			std::size_t const &seq_length,
			t_vector_source &vector_source
		)
		{
			reader_cb <t_vector_source>::handle_sequence(seq_ptr, seq_length, vector_source);
		}
	};
}}

#endif

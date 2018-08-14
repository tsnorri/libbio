/*
 * Copyright (c) 2016-2017 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_FASTA_READER_HH
#define LIBBIO_FASTA_READER_HH

#include <atomic>
#include <iostream>
#include <iterator>
#include <libbio/assert.hh>
#include <libbio/util.hh>
#include <libbio/vector_source.hh>


namespace libbio { namespace detail {
	
	template <typename t_vector_source>
	struct fasta_reader_cb {
		void handle_sequence(
			std::string const &identifier,
			std::unique_ptr <typename t_vector_source::vector_type> &seq,
			std::size_t const &seq_length,
			t_vector_source &vector_source
		)
		{
			vector_source.put_vector(seq);
		}
		
		void finish() {}
	};
}}


namespace libbio {
	
	template <
		typename t_vector_source,
		typename t_callback = detail::fasta_reader_cb <t_vector_source>,
		std::size_t t_initial_size = 128
	>
	class fasta_reader
	{
	protected:
		typedef t_vector_source						vector_source;
		typedef typename vector_source::vector_type	vector_type;
		
	protected:
		std::atomic_size_t							m_lineno{0};
		
	public:
		std::size_t current_line() const { return m_lineno; }
		
		
		void read_from_stream(std::istream &stream, vector_source &vector_source, t_callback &cb)
		{
			m_lineno = 0;
			
			std::size_t const size(1024 * 1024);
			std::size_t seq_length(0);
			vector_type buffer(size, 0);
			std::unique_ptr <vector_type> seq;
			std::string current_identifier;
			
			cb.start();
			
			try
			{
				std::uint32_t i(0);
				
				vector_source.get_vector(seq);
				if (t_initial_size && seq->size() < t_initial_size)
					seq->resize(t_initial_size);
				
				// Get a char pointer to the data part of the buffer.
				// This is safe because the element width is 8.
				char *buffer_data(reinterpret_cast <char *>(buffer.data()));
				
				while (stream.getline(buffer_data, size - 1, '\n'))
				{
					++m_lineno;
					
					// Discard comments.
					auto const first(buffer_data[0]);
					if (';' == first)
						continue;
					
					if ('>' == first)
					{
						// Reset the sequence length and get a new vector.
						if (seq_length)
						{
							assert(seq.get());
							cb.handle_sequence(current_identifier, seq, seq_length, vector_source);
							assert(nullptr == seq.get());
							seq_length = 0;
							
							vector_source.get_vector(seq);
							if (t_initial_size && seq->size() < t_initial_size)
								seq->resize(t_initial_size);
						}
						
						current_identifier = std::string(1 + buffer_data);
						continue;
					}
					
					// Get the number of characters extracted by the last input operation.
					// Delimiter is counted in gcount.
					std::streamsize const count(stream.gcount() - 1);
					
					while (true)
					{
						auto const capacity(seq->size());
						if (seq_length + count <= capacity)
							break;
						else
						{
							if (2 * capacity < capacity)
								libbio_fail("Can't reserve more space.");
							
							// std::vector may reserve more than 2 * capacity (using reserve),
							// sdsl::int_vector reserves the exact amount.
							// Make sure that at least some space is reserved.
							auto new_size(2 * capacity);
							if (new_size < 64)
								new_size = 64;
							seq->resize(new_size);
						}
					}

					auto const it(buffer.begin());
					std::copy(it, it + count, seq->begin() + seq_length);
					seq_length += count;
				}
				
				if (seq_length)
				{
					assert(seq.get());
					cb.handle_sequence(current_identifier, seq, seq_length, vector_source);
					assert(nullptr == seq.get());
				}
			}
			catch (std::ios_base::failure &exc)
			{
				if (stream.eof())
				{
					if (seq_length)
					{
						assert(seq.get());
						cb.handle_sequence(current_identifier, seq, seq_length, vector_source);
					}
				}
				else
				{
					throw (exc);
				}
			}
			
			cb.finish();
		}
	};
}

#endif

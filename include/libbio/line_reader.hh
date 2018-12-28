/*
 * Copyright (c) 2016-2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_LINE_READER_HH
#define LIBBIO_LINE_READER_HH

#include <algorithm>
#include <boost/iostreams/concepts.hpp>
#include <iostream>
#include <iterator>
#include <libbio/assert.hh>
#include <libbio/vector_source.hh>


namespace libbio { namespace detail {
	
	template <typename t_vector_source>
	struct line_reader_cb {
		void handle_sequence(
			uint32_t line,
			std::unique_ptr <typename t_vector_source::vector_type> &seq,
			std::size_t const &seq_length,
			t_vector_source &vector_source
		)
		{
			vector_source.put_vector(seq);
		}
		
		void start() {}
		void finish() {}
	};
}}


namespace libbio {
	
	template <
		typename t_vector_source,
		typename t_callback = detail::line_reader_cb <t_vector_source>,
		std::size_t t_initial_size = 128
	>
	class line_reader
	{
	protected:
		typedef t_vector_source						vector_source;
		typedef typename vector_source::vector_type	vector_type;
		
	public:
		void read_from_stream(std::istream &stream, vector_source &vector_source, t_callback &cb) const
		{
			std::size_t const size(1024 * 1024);
			vector_type buffer(size, 0);
			std::unique_ptr <vector_type> seq;
			uint32_t line_no(0);
			
			cb.start();
			
			try
			{
				// FIXME: make it possible to use the given buffer (seq) directly.
				
				// Get a char pointer to the data part of the buffer.
				// This is safe (w.r.t. alignment) because the element width is 8.
				char *buffer_data(reinterpret_cast <char *>(buffer.data()));
				
				char const delim('\n');
				while (stream.getline(buffer_data, size - 1, delim))
				{
					++line_no;
					
					// Get a buffer if needed.
					if (!seq.get())
					{
						vector_source.get_vector(seq);
						if (t_initial_size && seq->size() < t_initial_size)
							seq->resize(t_initial_size);
					}
					
					// Get the number of characters extracted by the last input operation.
					// Delimiter is counted in gcount.
					std::streamsize const gcount(stream.gcount());
					std::streamsize const has_delim(delim == *(buffer_data + gcount - 1) ? 1 : 0);
					std::streamsize const count(stream.gcount() - has_delim);
					
					while (true)
					{
						auto capacity(seq->size());
						if (is_lte(count, capacity))
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
					std::copy_n(it, count, seq->begin());
					
					// If there was data, handle it.
					if (count)
					{
						libbio_assert(seq.get());
						cb.handle_sequence(line_no, seq, count, vector_source);
						libbio_assert(nullptr == seq.get());
					}
				}
			}
			catch (std::ios_base::failure &exc)
			{
				if (!stream.eof())
					throw (exc);
			}
			
			cb.finish();
		}
	};
}

#endif

/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_SDSL_BOOST_SERIALIZATION_HH
#	define LIBBIO_SDSL_BOOST_SERIALIZATION_HH

#	ifdef LIBBIO_HAVE_SDSL


#		include <boost/serialization/binary_object.hpp>
#		include <climits>
#		include <sdsl/int_vector.hpp>


// Boost.Serialize requirements for sdsl::int_vector.

namespace boost { namespace serialization {
	
	template <typename t_archive, std::uint8_t t_width>
	void save(t_archive &ar, sdsl::int_vector <t_width> const &vector, unsigned int const version)
	{
		std::uint64_t capacity(vector.capacity());
		std::uint64_t const bit_size(vector.bit_size());
		auto const *data(vector.data());
		
		ar & capacity;
		ar & bit_size;
		ar & make_binary_object(data, vector.bit_capacity() / CHAR_BIT); // binary_object stores a sequence of bytes.
	}
	
	
	template <typename t_archive, std::uint8_t t_width>
	void load(t_archive &ar, sdsl::int_vector <t_width> &vector, unsigned int const version)
	{
		std::uint64_t capacity(0);
		std::uint64_t bit_size(0);
		binary_object obj(nullptr, 0); // No default constructor.
		
		ar & capacity;
		ar & bit_size;
		ar & obj;
		
		libbio_always_assert(vector.bit_capacity() / CHAR_BIT == obj.m_size);
		
		vector.reserve(capacity);
		vector.bit_resize(bit_size);
		
		// binary_object stores the data in a void *. Play safe and assume that
		// the data is byte-aligned.
		auto *dst(vector.data());
		auto *src(obj.m_t);
		std::copy_n(reinterpret_cast <char const *>(src), obj.m_size, reinterpret_cast <char *>(dst));
	}
	
	
	template <typename t_archive, std::uint8_t t_width>
	void serialize(t_archive &ar, sdsl::int_vector <t_width> &vector, unsigned int version)
	{
		// See <boost/serialization/split_free.hpp>; the definition of BOOST_SERIALIZATION_SPLIT_FREE
		// is essentially the same as this function.
		split_free(ar, vector, version);
	}
}}

#	endif
#endif

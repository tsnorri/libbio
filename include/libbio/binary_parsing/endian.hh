/*
 * Copyright (c) 2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_BINARY_PARSING_ENDIAN_HH
#define LIBBIO_BINARY_PARSING_ENDIAN_HH

#include <boost/endian.hpp>
#include <stdexcept>


namespace libbio::binary_parsing {
	
	enum class endian
	{
		unspecified,
		big,
		little
	};
}


namespace libbio::binary_parsing::detail {
	
	constexpr boost::endian::order to_boost_endian_order(endian const ee)
	{
		switch (ee)
		{
			case endian::unspecified:	throw std::runtime_error("Unexpected value");
			case endian::little:		return boost::endian::order::little;
			case endian::big:			return boost::endian::order::big;
		}
	}
}

#endif

/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <chrono>
#include <ctime>
#include <iomanip>
#include <libbio/utility.hh>
#include <sstream>


namespace libbio {
	
	void log_time(std::ostream &stream)
	{
		auto const time(std::chrono::system_clock::now());
		auto const ct(std::chrono::system_clock::to_time_t(time));
		struct tm ctm;
		localtime_r(&ct, &ctm);
		
		stream
		<< '['
		<< std::setw(2) << std::setfill('0') << ctm.tm_hour << ':'
		<< std::setw(2) << std::setfill('0') << ctm.tm_min << ':'
		<< std::setw(2) << std::setfill('0') << ctm.tm_sec
		<< "] ";
	}
	
	
	std::string copy_time()
	{
		std::stringstream sstream;
		log_time(sstream);
		return sstream.str();
	}
	
	
	std::size_t strlen_utf8(std::string const &str)
	{
		std::size_t retval(0);
		for (auto const c : str)
		{
			// Count the first bytes of each code point.
			// Byte 1 will have either 0b0 or 0b11 in the most significant end
			// while bytes 2, 3 and 4 will have 0b10.
			if (0x80 != (0xc0 & c))
				++retval;
		}
		
		return retval;
	}
}

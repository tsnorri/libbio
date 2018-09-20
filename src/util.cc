/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <chrono>
#include <ctime>
#include <iomanip>
#include <libbio/util.hh>


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
}

/*
 * Copyright (c) 2017-2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_PROGRESS_BAR_HH
#define LIBBIO_PROGRESS_BAR_HH

#include <chrono>
#include <ostream>
#include <string>


namespace libbio {
	void progress_bar(
		std::ostream &stream,
		float const value,
		std::size_t const length,
		std::size_t const pad,
		std::string const &title,
		std::chrono::time_point <std::chrono::steady_clock> start_time
	);
}

#endif

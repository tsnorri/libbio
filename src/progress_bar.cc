/*
 * Copyright (c) 2017-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <boost/format.hpp>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <libbio/assert.hh>
#include <libbio/progress_bar.hh>
#include <ostream>
#include <string>

namespace chrono = std::chrono;


namespace {
	enum {
		PERCENT_WIDTH = 5,
		TIME_WIDTH = 9
	};
}


namespace libbio {

	void progress_bar(
		std::ostream &stream,
		float const value,
		std::size_t length,
		std::size_t const pad,
		std::string const &title,
		chrono::time_point <chrono::steady_clock> start_time
	)
	{
		std::string const blocks[]{"▏", "▎", "▍", "▌", "▋", "▊", "▉", "█"};

		libbio_assert(0.0f <= value);
		libbio_assert(value <= 1.0f);

		if (PERCENT_WIDTH < length)
		{
			length -= PERCENT_WIDTH;

			stream << "\33[1G" << title;
			//stream << "\r\33[K" << title;
			for (std::size_t i(0); i < pad; ++i)
				stream << ' ';

			// Output the elapsed time.
			if (TIME_WIDTH < length)
			{
				length -= TIME_WIDTH;
				auto const current_time(chrono::steady_clock::now());
				chrono::duration <double> const elapsed_seconds(current_time - start_time);
				auto const seconds(chrono::duration_cast <chrono::seconds> (elapsed_seconds).count() % 60);
				auto const minutes(chrono::duration_cast <chrono::minutes> (elapsed_seconds).count() % 60);
				auto const hours(chrono::duration_cast <chrono::hours> (elapsed_seconds).count());
				stream << (boost::format("%02d:%02d:%02d ") % hours % minutes % seconds);
			}

			// Output the progress bar.
			auto const v(value * length);
			auto const integral(std::floor(v));
			auto const fraction(v - integral);

			stream << " |";
			for (std::size_t i(0); i < integral; ++i)
				stream << "█";

			if (v != integral)
			{
				// Divide the fraction by the block size, 1/8th.
				auto const block_idx(static_cast <std::size_t>(std::floor(fraction / 0.125)));
				stream << blocks[block_idx];
			}

			for (std::size_t i(1 + integral); i < length; ++i)
				stream << ' ';

			stream << "| ";
		}

		// Display the value as percentage.
		stream << (boost::format("% 4.0f%%") % (100.0 * value)) << std::flush;
	}
}

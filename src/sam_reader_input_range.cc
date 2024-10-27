/*
 * Copyright (c) 2023-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <libbio/file_handle.hh>
#include <libbio/sam/input_range.hh>
#include <vector>

namespace lb	= libbio;
namespace sam	= libbio::sam;


namespace {

	bool do_update(sam::input_range_base &ir, std::vector <char> &buffer, lb::file_handle &fh)
	{
		auto const size(fh.read(buffer.size(), buffer.data()));
		if (0 == size)
		{
			ir.it = nullptr;
			ir.sentinel = nullptr;
			return false;
		}

		ir.it = buffer.data();
		ir.sentinel = ir.it + size;
		return true;
	}
}


namespace libbio::sam {

	bool file_handle_input_range::update()
	{
		return do_update(*this, buffer, fh);
	}


	bool file_handle_input_range_::update()
	{
		return do_update(*this, buffer, fh);
	}
}

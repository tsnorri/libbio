/*
 * Copyright (c) 2023 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <libbio/sam/input_range.hh>


namespace libbio { namespace sam {
	
	bool file_handle_input_range::update()
	{
		auto const size(fh.read(buffer.size(), buffer.data()));
		if (0 == size)
		{
			it = nullptr;
			sentinel = nullptr;
			return false;
		}
		
		it = buffer.data();
		sentinel = it + size;
		return true;
	}
}}

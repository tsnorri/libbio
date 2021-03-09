/*
 * Copyright (c) 2021 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <cstdio>
#include <iostream>
#include <unistd.h>
#include <libbio/file_handle.hh>


namespace libbio {
	
	file_handle::~file_handle()
	{
		if (-1 != m_fd)
		{
			auto const res(close(m_fd));
			if (0 != res)
				std::cerr << "ERROR: unable to close file handle: " << strerror(errno) << '\n';
		}
	}
}

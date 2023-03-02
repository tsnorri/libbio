/*
 * Copyright (c) 2023 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_LOG_MEMORY_USAGE_HH
#define LIBBIO_LOG_MEMORY_USAGE_HH

namespace libbio {

	void setup_allocated_memory_logging_();

	inline void setup_allocated_memory_logging()
	{
#if defined(LIBBIO_LOG_ALLOCATED_MEMORY) && LIBBIO_LOG_ALLOCATED_MEMORY
		setup_allocated_memory_logging_();
#endif
	}
}

#endif

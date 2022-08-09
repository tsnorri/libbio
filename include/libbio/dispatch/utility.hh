/*
 * Copyright (c) 2022 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_DISPATCH_DISPATCH_UTILITY_HH
#define LIBBIO_DISPATCH_DISPATCH_UTILITY_HH

#include <dispatch/dispatch.h>


namespace libbio {

	void install_dispatch_sigchld_handler(dispatch_queue_t queue, dispatch_block_t block);
}

#endif

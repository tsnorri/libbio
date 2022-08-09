/*
 * Copyright (c) 2022 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <libbio/dispatch/utility.hh>


namespace libbio {
	
	void install_dispatch_sigchld_handler(dispatch_queue_t queue, dispatch_block_t block)
	{
		static dispatch_once_t once_token;
		static dispatch_source_t signal_source;
		
		dispatch_once(&once_token, ^{
			signal_source = dispatch_source_create(DISPATCH_SOURCE_TYPE_SIGNAL, SIGCHLD, 0, queue);
			dispatch_source_set_event_handler(signal_source, block);
			dispatch_resume(signal_source);
		});
	}
}

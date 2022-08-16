/*
 * Copyright (c) 2022 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_DISPATCH_DISPATCH_UTILITY_HH
#define LIBBIO_DISPATCH_DISPATCH_UTILITY_HH

#include <dispatch/dispatch.h>


namespace libbio {
	
	struct sigchld_handler
	{
		virtual ~sigchld_handler() {}
		virtual void child_did_exit_with_nonzero_status(pid_t const pid, int const exit_status, char const *reason) = 0;
		virtual void child_received_signal(pid_t const pid, int const signal_number) = 0;
		virtual void finish_handling(bool const did_report_error) = 0;
	};

	void install_dispatch_sigchld_handler(dispatch_queue_t queue, dispatch_block_t block);
	void install_dispatch_sigchld_handler(dispatch_queue_t queue, sigchld_handler &handler);
}

#endif

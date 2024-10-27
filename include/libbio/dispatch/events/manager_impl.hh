/*
 * Copyright (c) 2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_DISPATCH_EVENTS_MANAGER_IMPL_HH
#define LIBBIO_DISPATCH_EVENTS_MANAGER_IMPL_HH

#include <libbio/dispatch/events/signal_source.hh>
#include <libbio/dispatch/queue.hh>

#	ifdef __linux__
#		include <libbio/dispatch/events/platform/manager_linux.hh>
namespace libbio::dispatch::events {
	typedef platform::linux::manager		manager;
	typedef platform::linux::signal_mask	signal_mask;
}
#	else
#		include <libbio/dispatch/events/platform/manager_kqueue.hh>
namespace libbio::dispatch::events {
	typedef platform::kqueue::manager		manager;
	typedef platform::kqueue::signal_mask	signal_mask;
}
#	endif


namespace libbio::dispatch::events {

	void install_sigchld_handler(manager &mgr, queue &qq, sigchld_handler &handler);
}

#endif

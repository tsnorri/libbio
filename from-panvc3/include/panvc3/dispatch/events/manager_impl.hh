/*
 * Copyright (c) 2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef PANVC3_DISPATCH_EVENTS_MANAGER_IMPL_HH
#define PANVC3_DISPATCH_EVENTS_MANAGER_IMPL_HH

#	ifdef __linux__
#		include <panvc3/dispatch/events/platform/manager_linux.hh>
namespace panvc3::dispatch::events {
	typedef ::panvc3::dispatch::events::platform::linux::manager manager;
}
#	else
#		include <panvc3/dispatch/events/platform/manager_kqueue.hh>
namespace panvc3::dispatch::events {
	typedef ::panvc3::dispatch::events::platform::kqueue::manager manager;
}
#	endif
#endif

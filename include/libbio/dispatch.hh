/*
 * Copyright (c) 2023-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_DISPATCH_HH
#define LIBBIO_DISPATCH_HH

// Ownership Semantics
// ===================
// – The library user owns the queues, the groups, the thread pool and the event manager.
// – The event manager owns the event sources.
// – Task ownership depends on the task type.
//
// Other things that are noteworthy
// ================================
// – A barrier is guaranteed to stop a queue cleanly if it calls thread_pool::stop() synchronously on the thread in which its task is executed.

#include <libbio/dispatch/barrier.hh>		// IWYU pragma: export
#include <libbio/dispatch/group.hh>			// IWYU pragma: export
#include <libbio/dispatch/queue.hh>			// IWYU pragma: export
#include <libbio/dispatch/task_decl.hh>		// IWYU pragma: export
#include <libbio/dispatch/task_def.hh>		// IWYU pragma: export
#include <libbio/dispatch/thread_pool.hh>	// IWYU pragma: export

#endif

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

#include <libbio/dispatch/barrier.hh>
#include <libbio/dispatch/group.hh>
#include <libbio/dispatch/queue.hh>
#include <libbio/dispatch/task_decl.hh>
#include <libbio/dispatch/task_def.hh>

#endif

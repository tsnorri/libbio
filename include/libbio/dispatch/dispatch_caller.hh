/*
 * Copyright (c) 2016-2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_DISPATCH_DISPATCH_CALLER_HH
#define LIBBIO_DISPATCH_DISPATCH_CALLER_HH

#include <dispatch/dispatch.h>
#include <libbio/assert.hh>


namespace libbio { namespace detail {
	
	template <typename t_owner, void(t_owner::*t_fn)()>
	void call_member_function(void *ctx)
	{
		t_owner *owner(static_cast <t_owner *>(ctx));
		(owner->*t_fn)();
	}
}}


namespace libbio {
	// Allow passing pointer-to-member-function to dispatch_async_f without std::function.
	template <typename t_owner>
	class dispatch_caller
	{
	protected:
		t_owner	*m_owner{nullptr};
		
	public:
		dispatch_caller(t_owner *owner): m_owner(owner) { libbio_assert(m_owner); }
		
		template <void(t_owner::*t_fn)()>
		void async(dispatch_queue_t queue)
		{
			dispatch_async_f(queue, m_owner, detail::call_member_function <t_owner, t_fn>);
		}
		
		template <void(t_owner::*t_fn)()>
		void sync(dispatch_queue_t queue)
		{
			dispatch_sync_f(queue, m_owner, detail::call_member_function <t_owner, t_fn>);
		}
		
		template <void(t_owner::*t_fn)()>
		void barrier_async(dispatch_queue_t queue)
		{
			dispatch_barrier_async_f(queue, m_owner, detail::call_member_function <t_owner, t_fn>);
		}
		
		template <void(t_owner::*t_fn)()>
		void group_async(dispatch_group_t group, dispatch_queue_t queue)
		{
			dispatch_group_async_f(group, queue, m_owner, detail::call_member_function <t_owner, t_fn>);
		}
		
		template <void(t_owner::*t_fn)()>
		void source_set_event_handler(dispatch_source_t source)
		{
			dispatch_set_context(source, m_owner);
			dispatch_source_set_event_handler_f(source, detail::call_member_function <t_owner, t_fn>);
		}
	};
	
	template <typename t_owner>
	dispatch_caller <t_owner> dispatch(t_owner *owner)
	{
		return dispatch_caller <t_owner>(owner);
	}
}

#endif

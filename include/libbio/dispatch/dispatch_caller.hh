/*
 * Copyright (c) 2016-2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_DISPATCH_DISPATCH_CALLER_HH
#define LIBBIO_DISPATCH_DISPATCH_CALLER_HH

#include <dispatch/dispatch.h>
#include <libbio/assert.hh>


namespace libbio { namespace detail {
	
	template <typename t_owner, bool = std::is_invocable_v <t_owner>> // Check for operator().
	struct dispatch_caller_default_fn
	{
		inline static constexpr void(t_owner::*value)(){nullptr}; // No default target.
	};
	
	template <typename t_owner>
	struct dispatch_caller_default_fn <t_owner, true>
	{
		inline static constexpr void(t_owner::*value)(){&t_owner::operator()}; // Call operator().
	};
	
	template <typename t_owner>
	inline constexpr void(t_owner::*dispatch_caller_default_fn_v)() = dispatch_caller_default_fn <t_owner>::value;
	
	
	template <typename t_owner, void(t_owner::*t_fn)()>
	requires (nullptr != t_fn)
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
	public:
		typedef t_owner owner_type;
		
	protected:
		t_owner	*m_owner{};
		
	public:
		dispatch_caller(owner_type &owner): m_owner(&owner) {}
		
		template <void(owner_type::*t_fn)() = detail::dispatch_caller_default_fn_v <owner_type>>
		requires (nullptr != t_fn)
		void async(dispatch_queue_t queue)
		{
			dispatch_async_f(queue, m_owner, detail::call_member_function <owner_type, t_fn>);
		}
		
		template <void(owner_type::*t_fn)() = detail::dispatch_caller_default_fn_v <owner_type>>
		requires (nullptr != t_fn)
		void sync(dispatch_queue_t queue)
		{
			dispatch_sync_f(queue, m_owner, detail::call_member_function <owner_type, t_fn>);
		}
		
		template <void(owner_type::*t_fn)() = detail::dispatch_caller_default_fn_v <owner_type>>
		requires (nullptr != t_fn)
		void barrier_async(dispatch_queue_t queue)
		{
			dispatch_barrier_async_f(queue, m_owner, detail::call_member_function <owner_type, t_fn>);
		}
		
		template <void(owner_type::*t_fn)() = detail::dispatch_caller_default_fn_v <owner_type>>
		requires (nullptr != t_fn)
		void group_async(dispatch_group_t group, dispatch_queue_t queue)
		{
			dispatch_group_async_f(group, queue, m_owner, detail::call_member_function <owner_type, t_fn>);
		}
		
		template <void(owner_type::*t_fn)() = detail::dispatch_caller_default_fn_v <owner_type>>
		requires (nullptr != t_fn)
		void group_notify(dispatch_group_t group, dispatch_queue_t queue)
		{
			dispatch_group_notify_f(group, queue, m_owner, detail::call_member_function <owner_type, t_fn>);
		}
		
		template <void(owner_type::*t_fn)() = detail::dispatch_caller_default_fn_v <owner_type>>
		requires (nullptr != t_fn)
		void source_set_event_handler(dispatch_source_t source)
		{
			dispatch_set_context(source, m_owner);
			dispatch_source_set_event_handler_f(source, detail::call_member_function <owner_type, t_fn>);
		}
	};
	
	
	// Safe if owner exists until the block has been run.
	template <typename t_owner>
	dispatch_caller <t_owner> dispatch(t_owner &owner)
	{
		return dispatch_caller <t_owner>(owner);
	}
}

#endif

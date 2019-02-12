/*
 * Copyright (c) 2016-2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_DISPATCH_DISPATCH_FN_HH
#define LIBBIO_DISPATCH_DISPATCH_FN_HH

#include <dispatch/dispatch.h>
#include <iostream>
#include <libbio/assert.hh>
#include <stdexcept>


namespace libbio { namespace detail {
	
	// A virtual destructor is required in dispatch_source_set_event_handler_fn.
	class dispatch_fn_context_base
	{
	public:
		virtual ~dispatch_fn_context_base() {}
	};
	
	template <typename Fn>
	class dispatch_fn_context final : public dispatch_fn_context_base
	{
	public:
		typedef Fn function_type;
		
	protected:
		function_type m_fn;
	
	protected:
		static inline void do_call_fn(dispatch_fn_context &ctx)
		{
			try
			{
				ctx.m_fn();
			}
			catch (std::exception const &exc)
			{
				std::cerr << "Caught exception: " << exc.what() << std::endl;
			}
			catch (...)
			{
				std::cerr << "Caught non-std::exception." << std::endl;
			}
		}
	
	public:
		dispatch_fn_context(Fn &&fn):
			m_fn(std::move(fn))
		{
		}
		
		virtual ~dispatch_fn_context() {}
		
		static void cleanup(void *dispatch_context)
		{
			libbio_assert(dispatch_context);
			auto *ctx(reinterpret_cast <dispatch_fn_context *>(dispatch_context));
			delete ctx;
		}
		
		static void call_fn_no_delete(void *dispatch_context)
		{
			libbio_assert(dispatch_context);
			auto *ctx(reinterpret_cast <dispatch_fn_context *>(dispatch_context));
			do_call_fn(*ctx);
		}
		
		static void call_fn(void *dispatch_context)
		{
			libbio_assert(dispatch_context);
			auto *ctx(reinterpret_cast <dispatch_fn_context *>(dispatch_context));
			do_call_fn(*ctx);
			delete ctx;
		}
	};
}}


namespace libbio {
	
	template <typename Fn>
	void dispatch_async_fn(dispatch_queue_t queue, Fn fn)
	{
		// A new expression doesn't leak memory if the object construction throws an exception.
		typedef detail::dispatch_fn_context <Fn> context_type;
		auto *ctx(new context_type(std::move(fn)));
		dispatch_async_f(queue, ctx, &context_type::call_fn);
	}
	
	template <typename Fn>
	void dispatch_barrier_async_fn(dispatch_queue_t queue, Fn fn)
	{
		typedef detail::dispatch_fn_context <Fn> context_type;
		auto *ctx(new context_type(std::move(fn)));
		dispatch_barrier_async_f(queue, ctx, &context_type::call_fn);
	}
	
	template <typename Fn>
	void dispatch_group_async_fn(dispatch_group_t group, dispatch_queue_t queue, Fn fn)
	{
		typedef detail::dispatch_fn_context <Fn> context_type;
		auto *ctx(new context_type(std::move(fn)));
		dispatch_group_async_f(group, queue, ctx, &context_type::call_fn);
	}
	
	template <typename Fn>
	void dispatch_group_notify_fn(dispatch_group_t group, dispatch_queue_t queue, Fn fn)
	{
		typedef detail::dispatch_fn_context <Fn> context_type;
		auto *ctx(new context_type(std::move(fn)));
		dispatch_group_notify_f(group, queue, ctx, &context_type::call_fn);
	}
	
	template <typename Fn>
	void dispatch_sync_fn(dispatch_queue_t queue, Fn fn)
	{
		typedef detail::dispatch_fn_context <Fn> context_type;
		auto *ctx(new context_type(std::move(fn)));
		dispatch_sync_f(queue, ctx, &context_type::call_fn);
	}
	
	template <typename Fn>
	void dispatch_source_set_event_handler_fn(dispatch_source_t source, Fn fn)
	{
		typedef detail::dispatch_fn_context <Fn> context_type;
		
		// If the source has been cancelled, dispatch_get_context will return a dangling pointer.
		libbio_assert(!dispatch_source_testcancel(source));
		
		{
			// If there is an old context, deallocate it.
			auto *dispatch_context(dispatch_get_context(source));
			if (dispatch_context)
			{
				auto *ctx(reinterpret_cast <detail::dispatch_fn_context_base *>(dispatch_context));
				delete ctx;
			}
		}
		
		auto *new_ctx(new context_type(std::move(fn)));
		dispatch_set_context(source, new_ctx);
		dispatch_source_set_event_handler_f(source, &context_type::call_fn_no_delete);
		dispatch_source_set_cancel_handler_f(source, &context_type::cleanup);
	}
}

#endif

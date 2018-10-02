/*
 * Copyright (c) 2016-2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_DISPATCH_FN_HH
#define LIBBIO_DISPATCH_FN_HH

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <dispatch/dispatch.h>
#include <iostream>
#include <string>
#include <stdexcept>

#ifndef LIBBIO_USE_STD_PARALLEL_FOR_EACH
#	define LIBBIO_USE_STD_PARALLEL_FOR_EACH 0
#endif

#if LIBBIO_USE_STD_PARALLEL_FOR_EACH
#	include <execution>
#endif


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
			assert(dispatch_context);
			auto *ctx(reinterpret_cast <dispatch_fn_context *>(dispatch_context));
			delete ctx;
		}
		
		static void call_fn_no_delete(void *dispatch_context)
		{
			assert(dispatch_context);
			auto *ctx(reinterpret_cast <dispatch_fn_context *>(dispatch_context));
			do_call_fn(*ctx);
		}
		
		static void call_fn(void *dispatch_context)
		{
			assert(dispatch_context);
			auto *ctx(reinterpret_cast <dispatch_fn_context *>(dispatch_context));
			do_call_fn(*ctx);
			delete ctx;
		}
	};
	
	
	template <typename t_owner, void(t_owner::*t_fn)()>
	void call_member_function(void *ctx)
	{
		t_owner *owner(static_cast <t_owner *>(ctx));
		(owner->*t_fn)();
	}
	
	
	template <typename t_owner, void(t_owner::*t_fn)(std::size_t)>
	void call_member_function_dispatch_apply(void *ctx, std::size_t idx)
	{
		t_owner *owner(static_cast <t_owner *>(ctx));
		(owner->*t_fn)(idx);
	}
	
	
	template <
		typename t_range,
		typename Fn
	>
	class parallel_for_each_helper
	{
	protected:
		Fn					*m_fn{};	// Not owned.
		t_range				*m_range{};	// Not owned.
		std::size_t			m_count{};
		std::size_t			m_stride{};
		
	protected:
		void do_apply(std::size_t const idx)
		{
			// Call *m_fn for the current subrange.
			auto const start(idx * m_stride);
			auto const end(std::min(start + m_stride, m_count));
			if (start < end)
			{
				auto it(m_range->begin() + start);
				for (std::size_t i(start); i < end; ++i)
					(*m_fn)(*(m_range->begin() + i));
			}
		}
		
	public:
		parallel_for_each_helper(t_range &range, Fn &fn, std::size_t stride = 8):
			m_fn(&fn),
			m_range(&range),
			m_count(range.size()),
			m_stride(stride)
		{
		}
		
		void apply()
		{
			typedef parallel_for_each_helper helper_type;
			std::size_t const iterations(std::ceil(1.0 * m_count / m_stride));

			if (1 < iterations)
			{
				dispatch_apply_f(
					iterations,
#ifdef DISPATCH_APPLY_AUTO
					DISPATCH_APPLY_AUTO,
#else
					dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0),
#endif
					this,
					&call_member_function_dispatch_apply <helper_type, &helper_type::do_apply>
				);
			}
			else
			{
				auto it(m_range->begin());
				auto const end(m_range->end());
				while (it != end)
				{
					(*m_fn)(*it);
					++it;
				}
			}
		}
	};
}}


namespace libbio {
	// Allow passing pointer-to-member-function to dispatch_async_f without std::function.
	template <typename t_owner>
	class dispatch_caller
	{
	protected:
		t_owner	*m_owner{nullptr};
		
	public:
		dispatch_caller(t_owner *owner): m_owner(owner) { assert(m_owner); }
		
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
		assert(!dispatch_source_testcancel(source));
		
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
	
	template <typename Fn, typename t_range>
	void parallel_for_each(
		t_range &&range,
		Fn &&fn
	)
	{
#if LIBBIO_USE_STD_PARALLEL_FOR_EACH
		std::for_each(
			std::execution::par_unseq,
			range.begin(),
			range.end(),
			std::forward <Fn>(fn)
		);
#else
		detail::parallel_for_each_helper helper(range, fn);
		helper.apply();
#endif
	}


	template <typename Fn, typename t_range>
	void for_each(
		t_range &&range,
		Fn &&fn
	)
	{
		std::for_each(
			range.begin(),
			range.end(),
			std::forward <Fn>(fn)
		);
	}
	
	
	template <typename t_dispatch>
	class dispatch_ptr
	{
	protected:
		t_dispatch	m_ptr{};
		
	public:
		dispatch_ptr()
		{
		}
		
		dispatch_ptr(t_dispatch ptr, bool retain = false):
			m_ptr(ptr)
		{
			if (m_ptr && retain)
				dispatch_retain(m_ptr);
		}
		
		~dispatch_ptr()
		{
			if (m_ptr)
				dispatch_release(m_ptr);
		}
		
		dispatch_ptr(dispatch_ptr const &other):
			dispatch_ptr(other.m_ptr, true)
		{
		}
		
		dispatch_ptr(dispatch_ptr &&other):
			m_ptr(other.m_ptr)
		{
			other.m_ptr = nullptr;
		}
		
		bool operator==(dispatch_ptr const &other) const
		{
			return m_ptr == other.m_ptr;
		}
		
		bool operator!=(dispatch_ptr const &other) const
		{
			return !(*this == other);
		}
		
		dispatch_ptr &operator=(dispatch_ptr const &other) &
		{
			if (*this != other)
			{
				if (m_ptr)
					dispatch_release(m_ptr);
				
				m_ptr = other.m_ptr;
				dispatch_retain(m_ptr);
			}
			return *this;
		}
		
		dispatch_ptr &operator=(dispatch_ptr &&other) &
		{
			if (*this != other)
			{
				m_ptr = other.m_ptr;
				other.m_ptr = nullptr;
			}
			return *this;
		}
		
		t_dispatch operator*() { return m_ptr; }
		
		void reset(t_dispatch ptr = nullptr, bool retain = false)
		{
			if (m_ptr)
				dispatch_release(m_ptr);
			
			m_ptr = ptr;
			if (retain && m_ptr)
				dispatch_retain(m_ptr);
		}
	};
	
	
	template <typename t_dispatch>
	void swap(dispatch_ptr <t_dispatch> &lhs, dispatch_ptr <t_dispatch> &rhs)
	{
		using std::swap;
		swap(lhs.m_ptr, rhs.m_ptr);
	}
}

#endif

/*
 * Copyright (c) 2016-2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_DISPATCH_FOR_EACH_HH
#define LIBBIO_DISPATCH_FOR_EACH_HH

#include <algorithm>
#include <cmath>
#include <dispatch/dispatch.h>

#ifndef LIBBIO_USE_STD_PARALLEL_FOR_EACH
#	define LIBBIO_USE_STD_PARALLEL_FOR_EACH 0
#endif

#if LIBBIO_USE_STD_PARALLEL_FOR_EACH
#	include <execution>
#endif


namespace libbio { namespace detail {
	
	template <typename t_owner, void(t_owner::*t_fn)(std::size_t)>
	void call_member_function_dispatch_apply(void *ctx, std::size_t idx)
	{
		t_owner *owner(static_cast <t_owner *>(ctx));
		(owner->*t_fn)(idx);
	}
	
	
	template <typename Fn>
	class parallel_for_helper
	{
	protected:
		Fn			*m_fn{};
		std::size_t	m_count{};
		std::size_t	m_stride{};
		
	protected:
		void do_apply(std::size_t const idx)
		{
			// Call *m_fn for the current subrange.
			auto const start(idx * m_stride);
			auto const end(std::min(start + m_stride, m_count));
			if (start < end)
			{
				for (std::size_t i(start); i < end; ++i)
					(*m_fn)(i);
			}
		}
		
	public:
		parallel_for_helper(Fn &fn, std::size_t count, std::size_t stride = 8):
			m_fn(&fn),
			m_count(count),
			m_stride(stride)
		{
		}
		
		void apply()
		{
			typedef parallel_for_helper helper_type;
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
				for (std::size_t i(0); i < m_count; ++i)
					(*m_fn)(i);
			}
		}
	};
	
	
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
					(*m_fn)(*(m_range->begin() + i), i);
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
				std::size_t idx(0);
				while (it != end)
				{
					(*m_fn)(*it, idx++);
					++it;
				}
			}
		}
	};
}}


namespace libbio {
	
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
	void parallel_for_each(
		t_range &&range,
		std::size_t const stride,
		Fn && fn
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
		detail::parallel_for_each_helper helper(range, fn, stride);
		helper.apply();
#endif
	}
	
	
	template <typename Fn>
	void parallel_for(
		std::size_t const count,
		Fn && fn
	)
	{
		detail::parallel_for_helper helper(fn, count);
		helper.apply();
	}
	
	
	template <typename Fn>
	void parallel_for(
		std::size_t const count,
		std::size_t const stride,
		Fn && fn
	)
	{
		detail::parallel_for_helper helper(fn, count, stride);
		helper.apply();
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
}

#endif

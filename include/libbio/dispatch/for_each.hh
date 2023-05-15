/*
 * Copyright (c) 2016-2023 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_DISPATCH_FOR_EACH_HH
#define LIBBIO_DISPATCH_FOR_EACH_HH

#include <algorithm>
#include <cmath>
#include <libbio/dispatch/dispatch_compat.h>
#include <range/v3/range/primitives.hpp>
#include <range/v3/view/chunk.hpp>
#include <range/v3/view/enumerate.hpp>

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
	
	
	template <typename t_fn>
	class parallel_for_helper
	{
	protected:
		t_fn		&m_fn{};
		std::size_t	m_count{};
		std::size_t	m_stride{};
		
	protected:
		void do_apply(std::size_t const idx)
		{
			// Call m_fn for the current subrange.
			auto const start(idx * m_stride);
			auto const end(std::min(start + m_stride, m_count));
			if (start < end)
				m_fn(start, end);
		}
		
	public:
		parallel_for_helper(t_fn &fn, std::size_t count, std::size_t stride):
			m_fn(fn),
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
				m_fn(0, m_count);
			}
		}
	};
}}


namespace libbio {
	
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
}

#endif

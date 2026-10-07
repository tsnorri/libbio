/*
 * Copyright (c) 2026 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_DISPATCH_ALGORITHM_HH
#define LIBBIO_DISPATCH_ALGORITHM_HH

#include <cstdint>
#include <libbio/dispatch/group.hh>
#include <libbio/dispatch/queue.hh>
#include <libbio/dispatch/task_decl.hh>


namespace libbio::dispatch::detail {

	inline void wait_for_group(group &gg)
	{
#if LIBBIO_ENABLE_DISPATCH_FIBER_SUPPORT
		gg.wait_and_yield();
#else
		gg.wait();
#endif
	}


	template <typename t_fn>
	struct for_block_base
	{
		typedef std::uint64_t index_type;

		t_fn *fn{};
		index_type start{};
		index_type limit{};
	};


	template <typename t_fn, typename t_base = for_block_base <t_fn>>
	struct for_block : public t_base
	{
		typedef t_base::index_type index_type;

		void operator()() // Technically this is const since we access fn via a pointer to non-const type.
		{
			for (index_type ii{this->start}; ii < this->limit; ++ii)
				(*this->fn)(ii);
		}
	};


	template <typename t_context, typename t_fn, typename t_base = for_block_base <t_fn>>
	struct for_block_with_context : public t_base
	{
		typedef t_base::index_type index_type;

		t_context context{};

		void operator()() // Not const since we pass context which is intended to be mutable.
		{
			for (index_type ii{this->start}; ii < this->limit; ++ii)
				(*this->fn)(ii, context);
		}
	};


	template <typename t_enqueue>
	void for_(parallel_queue &queue, std::uint64_t const limit, std::uint64_t const block_size, t_enqueue &&enqueue)
	{
		group gg;

		std::uint64_t lb{};
		std::uint64_t rb{block_size};

		// FIXME: use a semaphore?
		while (rb < limit)
		{
			enqueue(gg, lb, rb);

			lb = rb;
			rb += block_size;
		}

		if (lb < limit)
			enqueue(gg, lb, limit);

		wait_for_group(gg);
	}
}


namespace libbio::dispatch {

	template <typename t_fn>
	void for_(parallel_queue &queue, std::uint64_t const limit, std::uint64_t const block_size, t_fn &&fn)
	{
		auto const enqueue{[&](group &gg, std::uint64_t lb, std::uint64_t rb){
			queue.group_async(gg, [fb = detail::for_block{&fn, lb, rb}] mutable {
				fb();
			});
		}};

		detail::for_(queue, limit, block_size, enqueue);
	}


	template <typename t_context, typename t_fn>
	void for_with_context(parallel_queue &queue, std::uint64_t const limit, std::uint64_t const block_size, t_fn &&fn)
	{
		auto const enqueue{[&](group &gg, std::uint64_t lb, std::uint64_t rb){
			queue.group_async(gg, [fb = detail::for_block_with_context <t_context, t_fn>{&fn, lb, rb}] mutable {
				fb();
			});
		}};

		detail::for_(queue, limit, block_size, enqueue);
	}
}

#endif

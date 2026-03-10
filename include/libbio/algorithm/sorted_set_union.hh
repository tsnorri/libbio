/*
 * Copyright (c) 2023-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_ALGORITHM_SORTED_SET_UNION_HH
#define LIBBIO_ALGORITHM_SORTED_SET_UNION_HH

#include <libbio/assert.hh>
#include <range/v3/range/access.hpp>
#include <type_traits>

namespace libbio::detail {

	struct sorted_set_default_cmp
	{
		template <typename t_lhs, typename t_rhs>
		bool operator()(t_lhs const &lhs, t_rhs const &rhs) const
		{
			// Should we use the spaceship operator instead?
			return lhs < rhs;
		}
	};


	template <typename t_output_it>
	struct sorted_set_union_output_iterator_handler
	{
		typedef t_output_it handle_remaining_return_type;

		t_output_it output_it;

		template <typename t_lhs_it, typename t_lhs_sentinel>
		handle_remaining_return_type handle_remaining_left(t_lhs_it &&it, t_lhs_sentinel &&end)
		{
			return std::copy(it, end, output_it);
		}

		template <typename t_rhs_it, typename t_rhs_sentinel>
		handle_remaining_return_type handle_remaining_right(t_rhs_it &&it, t_rhs_sentinel &&end)
		{
			return std::copy(it, end, output_it);
		}

		template <typename t_lhs_it>
		void handle_left(t_lhs_it &&it)
		{
			*output_it = *it;
			++output_it;
		}

		template <typename t_rhs_it>
		void handle_right(t_rhs_it &&it)
		{
			*output_it = *it;
			++output_it;
		}

		template <typename t_lhs_it, typename t_rhs_it>
		void handle_both(t_lhs_it &&lhs_it, t_rhs_it &&rhs_it)
		{
			*output_it = *lhs_it;
			++output_it;
		}
	};


	template <typename t_handler>
	struct sorted_set_union_callback_handler
	{
		typedef void handle_remaining_return_type;

		t_handler &handler;

		template <typename t_lhs_it, typename t_lhs_sentinel>
		void handle_remaining_left(t_lhs_it &&it, t_lhs_sentinel &&end)
		{
			while (it != end)
			{
				handler.handle_left(*it);
				++it;
			}
		}

		template <typename t_rhs_it, typename t_rhs_sentinel>
		void handle_remaining_right(t_rhs_it &&it, t_rhs_sentinel &&end)
		{
			while (it != end)
			{
				handler.handle_right(*it);
				++it;
			}
		}

		template <typename t_lhs_it>
		void handle_left(t_lhs_it &&it)
		{
			handler.handle_left(*it);
		}

		template <typename t_rhs_it>
		void handle_right(t_rhs_it &&it)
		{
			handler.handle_right(*it);
		}

		template <typename t_lhs_it, typename t_rhs_it>
		void handle_both(t_lhs_it &&lhs_it, t_rhs_it &&rhs_it)
		{
			handler.handle_both(*lhs_it, *rhs_it);
		}
	};


	template <
		typename t_lhs_it,
		typename t_lhs_sentinel,
		typename t_rhs_it,
		typename t_rhs_sentinel,
		typename t_handler,
		typename t_cmp,
		typename t_return = t_handler::handle_remaining_return_type
	>
	auto sorted_set_union(
		t_lhs_it lhs_it,
		t_lhs_sentinel lhs_end,
		t_rhs_it rhs_it,
		t_rhs_sentinel rhs_end,
		t_handler handler,
		t_cmp &&cmp
	) -> t_return
	{
		while (lhs_it != lhs_end)
		{
			if (rhs_it == rhs_end)
			{
				if constexpr (std::is_void_v <t_return>)
					handler.handle_remaining_left(lhs_it, lhs_end);
				else
					return handler.handle_remaining_left(lhs_it, lhs_end);
			}

			if (cmp(*lhs_it, *rhs_it))
			{
				handler.handle_left(lhs_it);
				++lhs_it;
				continue;
			}

			if (cmp(*rhs_it, *lhs_it))
			{
				handler.handle_right(rhs_it);
				++rhs_it;
				continue;
			}

			// *lhs_it == *rhs_it.
			handler.handle_both(lhs_it, rhs_it);
			++lhs_it;
			++rhs_it;
		}

		if constexpr (std::is_void_v <t_return>)
			handler.handle_remaining_right(rhs_it, rhs_end);
		else
			return handler.handle_remaining_right(rhs_it, rhs_end);
	}
}


namespace libbio {

	struct sorted_set_union_functor_handler_
	{
		struct handle_left_tag {};
		struct handle_right_tag {};
		struct handle_both_tag {};
	};


	template <typename t_base>
	struct sorted_set_union_functor_handler :
		public sorted_set_union_functor_handler_,
		public t_base
	{
		struct handle_left_tag {};
		struct handle_right_tag {};
		struct handle_both_tag {};

		using t_base::operator();

		constexpr sorted_set_union_functor_handler(t_base &&base):
			t_base{std::forward <t_base>(base)}
		{
		}

		template <typename t_value>
		void handle_left(t_value &&value)
		{
			operator()(std::forward <t_value>(value), handle_left_tag{});
		}

		template <typename t_value>
		void handle_right(t_value &&value)
		{
			operator()(std::forward <t_value>(value), handle_right_tag{});
		}

		template <typename t_lhs, typename t_rhs>
		void handle_both(t_lhs &&lhs, t_rhs &&rhs)
		{
			operator()(
				std::forward <t_lhs>(lhs),
				std::forward <t_rhs>(rhs),
				handle_both_tag{}
			);
		}
	};


	// Merge two sorted views to an output iterator. The elements in both views must be distinct w.r.t. cmp.
	template <typename t_lhs_it, typename t_lhs_sentinel, typename t_rhs_it, typename t_rhs_sentinel, typename t_dst, typename t_cmp>
	auto sorted_set_union(t_lhs_it lhs_it, t_lhs_sentinel lhs_end, t_rhs_it rhs_it, t_rhs_sentinel rhs_end, t_dst &&dst, t_cmp &&cmp)
	{
		typedef std::conditional_t <
			(
				std::output_iterator <t_dst, std::iter_value_t <t_lhs_it>> &&
				std::output_iterator <t_dst, std::iter_value_t <t_rhs_it>>
			),
			detail::sorted_set_union_output_iterator_handler <t_dst>,
			detail::sorted_set_union_callback_handler <t_dst>
		> handler_type;

		handler_type handler{dst};
		return detail::sorted_set_union(lhs_it, lhs_end, rhs_it, rhs_end, handler, cmp);
	}


	template <typename t_lhs_it, typename t_lhs_sentinel, typename t_rhs_it, typename t_rhs_sentinel, typename t_dst>
	auto sorted_set_union(t_lhs_it lhs_it, t_lhs_sentinel lhs_end, t_rhs_it rhs_it, t_rhs_sentinel rhs_end, t_dst dst)
	{
		detail::sorted_set_default_cmp cmp;
		return sorted_set_union(lhs_it, lhs_end, rhs_it, rhs_end, dst, cmp);
	}


	template <typename t_lhs_range, typename t_rhs_range, typename t_dst, typename t_cmp>
	auto sorted_set_union(t_lhs_range lhs_range, t_rhs_range rhs_range, t_dst &&dst, t_cmp &&cmp)
	{
		auto l_begin(ranges::begin(lhs_range));
		auto r_begin(ranges::begin(rhs_range));
		auto l_end(ranges::end(lhs_range));
		auto r_end(ranges::end(rhs_range));
		return sorted_set_union(l_begin, l_end, r_begin, r_end, dst, cmp);
	}


	template <typename t_lhs_range, typename t_rhs_range, typename t_dst>
	auto sorted_set_union(t_lhs_range lhs_range, t_rhs_range rhs_range, t_dst &&dst)
	{
		detail::sorted_set_default_cmp cmp;
		return sorted_set_union(lhs_range, rhs_range, dst, cmp);
	}
}

#endif

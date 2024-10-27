/*
 * Copyright (c) 2019-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_ALGORITHM_MERGE_PROJECTED_HH
#define LIBBIO_ALGORITHM_MERGE_PROJECTED_HH

#include <libbio/assert.hh>
#include <range/v3/range/access.hpp>
#include <type_traits>


namespace libbio::detail {

	template <typename t_lhs_proj_type, typename t_rhs_proj_type>
	class merge_projected_helper
	{
	public:
		merge_projected_helper() = default;


		template <typename t_lhs_it, typename t_lhs_end, typename t_rhs_it, typename t_rhs_end, typename t_output_it, typename t_proj_lhs, typename t_proj_rhs>
		void merge(
			t_lhs_it &l_begin,
			t_lhs_end const &l_end,
			t_rhs_it &r_begin,
			t_rhs_end const &r_end,
			t_output_it &output_it,
			t_proj_lhs &proj_lhs,
			t_proj_rhs &proj_rhs
		)
		{
			t_lhs_proj_type next_l{};
			t_rhs_proj_type next_r{};
			bool continue_l{true}, continue_r{true};

			if (l_begin == l_end)
			{
				continue_l = false;
				if (r_begin == r_end)
					return;

				next_r = proj_rhs(*r_begin, continue_r);
				if (!continue_r)
					return;
			}
			else if (r_begin == r_end)
			{
				continue_r = false;
				next_l = proj_lhs(*l_begin, continue_l);
				if (!continue_l)
					return;
			}
			else
			{
				next_l = proj_lhs(*l_begin, continue_l);
				next_r = proj_rhs(*r_begin, continue_r);

				while (continue_l && continue_r)
				{
					if (next_l < next_r)
					{
						*output_it = *l_begin;
						++l_begin;
						if (l_begin == l_end)
						{
							continue_l = false;
							break;
						}
						next_l = proj_lhs(*l_begin, continue_l);
					}
					else
					{
						*output_it = *r_begin;
						++r_begin;
						if (r_begin == r_end)
						{
							continue_r = false;
							break;
						}
						next_r = proj_rhs(*r_begin, continue_r);
					}
				}
			}

			while (continue_l)
			{
				*output_it = *l_begin;
				++l_begin;
				if (l_begin == l_end)
					break;

				next_l = proj_lhs(*l_begin, continue_l);
			}

			while (continue_r)
			{
				*output_it = *r_begin;
				++r_begin;
				if (r_begin == r_end)
					break;

				next_r = proj_rhs(*r_begin, continue_r);
			}
		}
	};
}


namespace libbio {

	// Merge views to an output iterator with an option to stop and a custom projection function.
	// Copies projection results.
	template <typename t_lhs_range, typename t_rhs_range, typename t_output_it, typename t_proj_lhs, typename t_proj_rhs>
	void merge_projected(t_lhs_range &&lhs_range, t_rhs_range &&rhs_range, t_output_it &&output_it, t_proj_lhs &&proj_lhs, t_proj_rhs &&proj_rhs)
	{
		auto l_begin(ranges::begin(lhs_range));
		auto r_begin(ranges::begin(rhs_range));
		auto l_end(ranges::end(lhs_range));
		auto r_end(ranges::end(rhs_range));

		// Determine the return types.
		typedef decltype(l_begin) lhs_iterator_type;
		typedef decltype(r_begin) rhs_iterator_type;
		typedef decltype(std::declval <lhs_iterator_type>().operator *()) lhs_deref_type;
		typedef decltype(std::declval <rhs_iterator_type>().operator *()) rhs_deref_type;
		typedef std::invoke_result_t <t_proj_lhs, lhs_deref_type, bool &> lhs_proj_type;
		typedef std::invoke_result_t <t_proj_rhs, rhs_deref_type, bool &> rhs_proj_type;

		detail::merge_projected_helper <lhs_proj_type, rhs_proj_type> helper;
		helper.merge(l_begin, l_end, r_begin, r_end, output_it, proj_lhs, proj_rhs);
	}


	template <typename t_lhs_range, typename t_rhs_range, typename t_output_it, typename t_proj_fn>
	void merge_projected(t_lhs_range &&lhs_range, t_rhs_range &&rhs_range, t_output_it &&output_it, t_proj_fn &&proj_fn)
	{
		merge_projected(lhs_range, rhs_range, output_it, proj_fn, proj_fn);
	}
}

#endif

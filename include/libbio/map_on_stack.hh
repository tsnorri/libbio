/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_MAP_ON_STACK_HH
#define LIBBIO_MAP_ON_STACK_HH


namespace libbio { namespace detail {
	
	template <int t_count>
	struct map_on_stack
	{
		// Map the first argument and pass the result as the last one.
		template <typename t_map, typename t_fn, typename t_first, typename ... t_args>
		void call(t_map &map, t_fn &fn, t_first &&first, t_args && ... args)
		{
			map(first, [&map, &fn, &args...](auto &&mapped){
				map_on_stack <t_count - 1> ctx;
				ctx.template call(map, fn, args..., mapped);
			});
		}
	};
	
	
	template <>
	struct map_on_stack <0>
	{
		// All arguments have been mapped, call fn.
		template <typename t_map, typename t_fn, typename ... t_args>
		void call(t_map &map, t_fn &fn, t_args && ... args)
		{
			fn(args...);
		}
	};
}}


namespace libbio {
	
	// Instantiate t_map t, then for each object o in t_args, call t(o).
	// Finally, call t_fn with the resulting objects.
	template <typename t_map, typename t_fn, typename ... t_args>
	void map_on_stack_fn(t_fn &&fn, t_args && ... args)
	{
		t_map map;
		detail::map_on_stack <sizeof...(t_args)> ctx;
		ctx.template call(map, fn, args...);
	}
	
	
	// Instantiate t_map t and t_fn, then for each object o in t_args, call t(o).
	// Finally, pass the objects to t_fn's operator().
	template <typename t_map, typename t_fn, typename ... t_args>
	void map_on_stack(t_args && ... args)
	{
		t_fn fn;
		map_on_stack_fn <t_map>(fn, args...);
	}
}

#endif

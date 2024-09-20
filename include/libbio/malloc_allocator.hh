/*
 * Copyright (c) 2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_MALLOC_ALLOCATOR_HH
#define LIBBIO_MALLOC_ALLOCATOR_HH

#include <cstdlib>
#include <new>		// std::bad_alloc


namespace libbio {
	
	template <typename t_type> 
	struct malloc_allocator
	{
		typedef t_type		value_type;
		typedef value_type	*pointer;
		typedef std::size_t	size_type;
	
		malloc_allocator() throw() {}
		malloc_allocator(malloc_allocator const &) throw() {}
	
		template <typename T>
		malloc_allocator(malloc_allocator <T> const &) throw() {}
	
		pointer allocate(size_type const size)
		{
			if (!size)
				return nullptr;
		
			auto *retval(static_cast <pointer>(::malloc(size * sizeof(t_type))));
			if (!retval)
				throw std::bad_alloc{};
		
			return retval;
		}
	
		void deallocate(pointer ptr, size_type) { ::free(ptr); }
	};
}

#endif

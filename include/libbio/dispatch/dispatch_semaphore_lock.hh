/*
 * Copyright (c) 2019-2023 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_DISPATCH_DISPATCH_SEMAPHORE_LOCK_HH
#define LIBBIO_DISPATCH_DISPATCH_SEMAPHORE_LOCK_HH

#include <libbio/dispatch/dispatch_compat.hh>
#include <libbio/dispatch/dispatch_ptr.hh>


namespace libbio {

	// Wrap dispatch_semaphore_t into a BasicLockable.
	class dispatch_semaphore_lock 
	{
	protected:
		dispatch_ptr <dispatch_semaphore_t> m_sema;
		
	public:
		dispatch_semaphore_lock()
		{
		}
		
		explicit dispatch_semaphore_lock(long value):
			m_sema(dispatch_semaphore_create(value), false)
		{
		}
		
		void lock()
		{
			dispatch_semaphore_wait(*m_sema, DISPATCH_TIME_FOREVER);
		}
		
		void unlock()
		{
			dispatch_semaphore_signal(*m_sema);
		}
	};
}

#endif

/*
 * Copyright (c) 2021-2023 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_DISPATCH_DISPATCH_GROUP_GUARD_HH
#define LIBBIO_DISPATCH_DISPATCH_GROUP_GUARD_HH

#include <libbio/dispatch/dispatch_compat.h>


namespace libbio {

	// RAII-style mechanism for leaving a dispatch group.
	class dispatch_group_nonowning_guard 
	{
	protected:
		dispatch_group_t	m_group{};
		
	public:
		explicit dispatch_group_nonowning_guard(dispatch_group_t group):
			m_group(group)
		{
			dispatch_group_enter(m_group);
		}
		
		dispatch_group_nonowning_guard(dispatch_group_nonowning_guard const &) = delete;
		dispatch_group_nonowning_guard &operator=(dispatch_group_nonowning_guard const &) = delete;
		
		~dispatch_group_nonowning_guard() { dispatch_group_leave(m_group); }
	};
}

#endif

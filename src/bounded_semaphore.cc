/*
 * Copyright (c) 2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <libbio/bounded_semaphore.hh>
#include <mutex>


namespace libbio {

	void bounded_semaphore::acquire()
	{
		{
			std::unique_lock lock{m_mutex};
			while (true)
			{
				if (m_counter)
				{
					--m_counter;
					break;
				}

				m_lower_cv.wait(lock);
			}
		}

		m_upper_cv.notify_one();
	}


	void bounded_semaphore::release()
	{
		{
			std::unique_lock lock{m_mutex};
			while (true)
			{
				if (m_counter < m_limit)
				{
					++m_counter;
					break;
				}

				m_upper_cv.wait(lock);
			}
		}

		m_lower_cv.notify_one();
	}
}

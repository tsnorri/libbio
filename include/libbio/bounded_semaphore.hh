/*
 * Copyright (c) 2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_BOUNDED_SEMAPHORE_HH
#define LIBBIO_BOUNDED_SEMAPHORE_HH

#include <condition_variable>
#include <cstddef>
#include <mutex>


namespace libbio {

	class bounded_semaphore
	{
	private:
		typedef std::ptrdiff_t	counter_type;

	private:
		std::condition_variable	m_lower_cv{};
		std::condition_variable	m_upper_cv{};
		std::mutex				m_mutex{};
		counter_type			m_counter{};
		counter_type			m_limit{};

	public:
		bounded_semaphore(counter_type counter, counter_type limit):
			m_counter(counter),
			m_limit(limit)
		{
		}

		bounded_semaphore(bounded_semaphore const &other) = delete;
		bounded_semaphore &operator=(bounded_semaphore const &) = delete;

		void acquire();
		void release();
	};
}

#endif

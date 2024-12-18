/*
 * Copyright (c) 2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_DISPATCH_EVENTS_PLATFORM_MANAGER_KQUEUE_HH
#define LIBBIO_DISPATCH_EVENTS_PLATFORM_MANAGER_KQUEUE_HH

#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <libbio/dispatch/events/file_descriptor_source.hh>
#include <libbio/dispatch/events/manager.hh>
#include <libbio/dispatch/events/signal_source.hh>
#include <libbio/dispatch/events/source.hh>
#include <libbio/dispatch/queue.hh>
#include <memory>
#include <mutex>
#include <sys/signal.h>
#include <unordered_map>
#include <utility>


namespace libbio::dispatch::events::platform::kqueue {

	typedef std::int16_t	filter_type;				// Type from struct kevent.


	struct source_key
	{
		int			value{};
		filter_type	filter{};

		source_key() = default;

		constexpr source_key(int const value_, filter_type const filter_):
			value(value_),
			filter(filter_)
		{
		}

		constexpr inline bool operator==(source_key const other) const { return value == other.value && filter == other.filter; }
	};
}


template <>
struct std::hash <libbio::dispatch::events::platform::kqueue::source_key>
{
	constexpr std::size_t operator()(libbio::dispatch::events::platform::kqueue::source_key const &sk) const noexcept
	{
		std::size_t retval(std::abs(sk.value));
		retval <<= 17;
		retval |= std::size_t(sk.value < 0 ? 0x1 : 0x0) << 16;
		retval |= std::bit_cast <std::uint16_t>(sk.filter);
		return retval;
	}
};


namespace libbio::dispatch::events::platform::kqueue {

	class signal_mask
	{
		std::unordered_map <
			int,
			struct sigaction
		>						m_old_actions;

	private:
		void remove_all_();

	public:
		signal_mask()
		{
		}

		~signal_mask() { remove_all_(); }

		void add(int sig);
		void remove(int sig);
		void remove_all();
	};


	class manager final : public events::manager_base
	{
		typedef events::detail::file_handle		file_handle;

		typedef std::unordered_multimap <
			source_key,
			std::shared_ptr <events::source>
		>										source_map;

		typedef std::pair <
			std::unique_lock <std::mutex>,
			bool
		>										remove_event_source_return_type;

		file_handle								m_kqueue_handle;
		source_map								m_sources;

		std::mutex								m_mutex{};

	public:
		~manager() { stop_and_wait(); } // Calls a virtual member function.
		void setup() override;
		void trigger_event(event_type const evt) override;

		file_descriptor_source &add_file_descriptor_read_event_source(
			file_descriptor_type const fd,
			queue &qq,
			file_descriptor_source::task_type tt
		) override;									// Thread-safe.

		file_descriptor_source &add_file_descriptor_write_event_source(
			file_descriptor_type const fd,
			queue &qq,
			file_descriptor_source::task_type tt
		) override;									// Thread-safe.

		signal_source &add_signal_event_source(
			signal_type const sig,
			queue &qq,
			signal_source::task_type tt
		) override;									// Thread-safe.

		void remove_file_descriptor_event_source(
			file_descriptor_source &es
		) override;									// Thread-safe.

		void remove_signal_event_source(
			signal_source &es
		) override;									// Thread-safe.

	private:
		void run_() override;
		remove_event_source_return_type remove_event_source(source &es, source_key const key);
	};
}

#endif

/*
 * Copyright (c) 2026 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_SYNCSTREAM_HH
#define LIBBIO_SYNCSTREAM_HH

#include <boost/iostreams/categories.hpp> // sink_tag
#include <boost/iostreams/concepts.hpp> // sink
#include <boost/iostreams/stream.hpp>
#include <ios>
#include <mutex>
#include <ostream>
#include <stdexcept>
#include <unordered_map>
#include <vector>


namespace libbio::detail {

	class pointer_registry
	{
	public:
		typedef std::lock_guard <std::mutex> lock_guard_type;

	private:
		typedef std::unordered_map<void *, std::mutex> mutex_map_type;
		mutex_map_type m_mutexes;

	public:
		static inline pointer_registry &shared_registry();
		inline void register_pointer(void *ptr);
		inline void unregister_pointer(void *ptr);
		inline lock_guard_type lock_pointer(void *ptr);
	};


	// Replacement for std::osyncstream since macOS Sequoia does not have one.
	class synchronized_stream_sink : public boost::iostreams::sink
	{
	private:
		typedef std::vector <char> container_type;

	public:
		typedef typename container_type::value_type	char_type;
		typedef boost::iostreams::sink_tag			category;

	private:
		std::ostream *m_stream{};
		pointer_registry *m_registry{};
		container_type m_buffer;

	public:
		explicit synchronized_stream_sink(std::ostream &stream, pointer_registry &registry = pointer_registry::shared_registry()):
			m_stream(&stream),
			m_registry(&registry)
		{
		}

		inline ~synchronized_stream_sink();
		inline std::streamsize write(char const *s, std::streamsize n);
	};


	pointer_registry &pointer_registry::shared_registry()
	{
		static pointer_registry registry;
		return registry;
	}


	void pointer_registry::register_pointer(void *ptr)
	{
		auto const res{m_mutexes.try_emplace(ptr)};
		if (!res.second)
			throw std::runtime_error("Pointer already registered");
	}


	void pointer_registry::unregister_pointer(void *ptr)
	{
		if (0 == m_mutexes.erase(ptr))
			throw std::runtime_error("Pointer not registered");
	}


	auto pointer_registry::lock_pointer(void *ptr) -> lock_guard_type
	{
		auto it{m_mutexes.find(ptr)};
		if (m_mutexes.end() == it)
			throw std::runtime_error("Pointer not registered");

		return lock_guard_type{it->second};
	}


	synchronized_stream_sink::~synchronized_stream_sink()
	{
		auto const lock{m_registry->lock_pointer(m_stream)};
		m_stream->write(m_buffer.data(), m_buffer.size());
	}


	std::streamsize synchronized_stream_sink::write(char const *buffer, std::streamsize size)
	{
		m_buffer.insert(m_buffer.end(), buffer, buffer + size);
		return size;
	}
}


namespace libbio {

#if defined(__cpp_lib_syncbuf)
	struct ostream_synchronizer
	{
		explicit ostream_synchronizer(std::ostream &os) {}
	};


	typedef std::osyncstream osyncstream;
#else
	// Guard-type class for making sure that the ostream has been registered.
	// The operations are not thread safe.
	class ostream_synchronizer
	{
		detail::pointer_registry *m_registry{};
		std::ostream *m_ostream{};

	public:
		explicit ostream_synchronizer(std::ostream &os, detail::pointer_registry &registry = detail::pointer_registry::shared_registry()):
			m_registry{&registry},
			m_ostream{&os}
		{
			m_registry->register_pointer(m_ostream);
		}

		~ostream_synchronizer()
		{
			m_registry->unregister_pointer(m_ostream);
		}
	};


	typedef boost::iostreams::stream <detail::synchronized_stream_sink> osyncstream;
#endif
}

#endif

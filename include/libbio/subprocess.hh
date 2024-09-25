/*
 * Copyright (c) 2022-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_SUBPROCESS_HH
#define LIBBIO_SUBPROCESS_HH

#include <cstdlib>
#include <libbio/assert.hh>
#include <libbio/bits.hh>
#include <libbio/file_handle.hh>
#include <libbio/utility/misc.hh>
#include <limits>
#include <range/v3/view/enumerate.hpp>
#include <tuple>


namespace libbio {
	
	class process_handle; // Fwd.
	
	
	enum class execution_status_type : std::uint8_t
	{
		no_error,
		file_descriptor_handling_failed,
		fork_failed,
		exec_failed
	};

	
	enum class subprocess_handle_spec : std::uint8_t
	{
		NONE		= 0x0,
		STDIN		= 0x1,
		STDOUT		= 0x2,
		STDERR		= 0x4,
		KEEP_STDERR	= 0x80
	};
	
	constexpr inline subprocess_handle_spec operator|(subprocess_handle_spec const lhs, subprocess_handle_spec const rhs)
	{
		return static_cast <subprocess_handle_spec>(to_underlying(lhs) | to_underlying(rhs));
	}
	
	constexpr inline subprocess_handle_spec operator&(subprocess_handle_spec const lhs, subprocess_handle_spec const rhs)
	{
		return static_cast <subprocess_handle_spec>(to_underlying(lhs) & to_underlying(rhs));
	}
	
	template <subprocess_handle_spec t_spec, subprocess_handle_spec t_h>
	constexpr inline bool has_handle() { return to_underlying(t_spec & t_h) ? true : false; }
	
	struct subprocess_status
	{
		execution_status_type	execution_status{execution_status_type::no_error};
		std::uint32_t			line{}; // For debugging.
		int						error{};

		constexpr operator bool() const { return execution_status_type::no_error == execution_status; }
		void output_status(std::ostream &os, bool const detailed) const;
	};
}


namespace libbio { namespace detail {
	
	[[nodiscard]] subprocess_status
	open_subprocess(process_handle &ph, char const * const args[], subprocess_handle_spec const handle_spec, libbio::file_handle *dst_handles); // nullptr-terminated list of arguments.
	
	
	[[nodiscard]] consteval std::size_t handle_count(subprocess_handle_spec const t_spec)
	{
		typedef std::underlying_type_t <subprocess_handle_spec> underlying_t;
		return bits::count_bits_set(underlying_t(0xf & to_underlying(t_spec)));
	}
	
	
	// Calculate the rank of curr in all.
	[[nodiscard]] constexpr std::size_t handle_index(subprocess_handle_spec const curr, subprocess_handle_spec const all)
	{
		typedef std::underlying_type_t <subprocess_handle_spec> underlying_t;
		
		// Require that there is a (non-zero) handle and it is in the given set.
		if (!to_underlying(curr & all))
			throw std::invalid_argument("Given handle specification is either zero or not in the set of all specifications");

		// Require that the given set contains only values that refer to instantiated handles.
		if (underlying_t(0xf0) & to_underlying(curr))
			throw std::invalid_argument("Given handle specification contains unexpected values");
		
		// Determine the 1-based index of the current value.
		auto const bit_idx(bits::highest_bit_set(to_underlying(curr)));
		
		// Make a mask of the lower bits.
		underlying_t mask(~underlying_t(0));
		mask >>= sizeof(underlying_t) * CHAR_BIT - bit_idx;
		mask >>= 0x1;
		
		// Count the lower bits.
		underlying_t const lower_bits(to_underlying(curr) & mask);
		auto const lower_count(bits::count_bits_set(lower_bits));
		
		return lower_count;
	}
	
	
	template <typename t_type>
	[[nodiscard]] constexpr inline decltype(auto) argument_data(t_type const &arg) { return std::data(arg); }
	
	[[nodiscard]] constexpr inline auto argument_data(char const *arg) { return arg; }
}}


namespace libbio {
	
	class process_handle
	{
		static_assert(std::numeric_limits <pid_t>::min() <= -1); // We use -1 as a magic value.
		
	public:
		enum class close_status
		{
			unknown,
			exit_called,
			terminated_by_signal,
			stopped_by_signal
		};
		
		typedef std::tuple <close_status, int, pid_t>	close_return_type;
		
	protected:
		pid_t				m_pid{-1};
		
	public:
		process_handle() = default;
		process_handle(process_handle const &) = delete;
		process_handle &operator=(process_handle const &) = delete;
		
		explicit process_handle(pid_t const pid):
			m_pid(pid)
		{
		}
		
		process_handle(process_handle &&other):
			m_pid(other.m_pid)
		{
			other.m_pid = -1;
		}
		
		inline process_handle &operator=(process_handle &&other) &;
		[[nodiscard]] pid_t pid() const { return m_pid; }
		close_return_type close();
		~process_handle() { if (-1 != m_pid) close(); }
	};
	
	
	[[nodiscard]] std::vector <std::string> parse_command_arguments(char const *args);
	
	
	template <subprocess_handle_spec t_handle_spec>
	class subprocess
	{
	public:
		constexpr static subprocess_handle_spec const handle_spec{t_handle_spec};

		typedef process_handle::close_return_type	close_return_type;
		typedef std::array <
			file_handle,
			detail::handle_count(t_handle_spec)
		>											handle_array;
		
	protected:
		process_handle	m_process{};	// Data member destructors called in reverse order; important for closing the pipe first.
		handle_array	m_handles{};
		
	protected:
		template <subprocess_handle_spec t_spec>
		[[nodiscard]] constexpr static bool has_handle() { return libbio::has_handle <t_handle_spec, t_spec>(); }
		
		[[nodiscard]] constexpr static std::size_t handle_index(subprocess_handle_spec const spec) { return detail::handle_index(spec, t_handle_spec); }
		template <subprocess_handle_spec t_spec> [[nodiscard]] constexpr file_handle       &handle()       { return std::get <handle_index(t_spec)>(m_handles); }
		template <subprocess_handle_spec t_spec> [[nodiscard]] constexpr file_handle const &handle() const { return std::get <handle_index(t_spec)>(m_handles); }
		
		subprocess_status open__(char const * const argv[], subprocess_handle_spec const spec);
		
		template <typename t_type>
		subprocess_status open_(t_type const &args, subprocess_handle_spec const spec);
		
	public:
		subprocess() = default;
		
		// The second argument is useful for having a set of subprocess instances of the same size.
		subprocess_status open(char const * const path, subprocess_handle_spec const spec = t_handle_spec);
		subprocess_status open(std::initializer_list <char const *> const &il, subprocess_handle_spec const spec = t_handle_spec) { return open_(il, spec); }
		subprocess_status open(std::vector <std::string> const &args, subprocess_handle_spec const spec = t_handle_spec) { return open_(args, spec); }
		subprocess_status parse_and_open(char const * const args, subprocess_handle_spec const spec = t_handle_spec);
		
		inline close_return_type close();
		
		[[nodiscard]] file_handle       &stdin_handle()        requires (has_handle <subprocess_handle_spec::STDIN>())  { return handle <subprocess_handle_spec::STDIN>(); }
		[[nodiscard]] file_handle const &stdin_handle()  const requires (has_handle <subprocess_handle_spec::STDIN>())  { return handle <subprocess_handle_spec::STDIN>(); }
		[[nodiscard]] file_handle       &stdout_handle()       requires (has_handle <subprocess_handle_spec::STDOUT>()) { return handle <subprocess_handle_spec::STDOUT>(); }
		[[nodiscard]] file_handle const &stdout_handle() const requires (has_handle <subprocess_handle_spec::STDOUT>()) { return handle <subprocess_handle_spec::STDOUT>(); }
		[[nodiscard]] file_handle       &stderr_handle()       requires (has_handle <subprocess_handle_spec::STDERR>()) { return handle <subprocess_handle_spec::STDERR>(); }
		[[nodiscard]] file_handle const &stderr_handle() const requires (has_handle <subprocess_handle_spec::STDERR>()) { return handle <subprocess_handle_spec::STDERR>(); }
	};
	
	
	process_handle &process_handle::operator=(process_handle &&other) &
	{
		if (this != &other)
		{
			using std::swap;
			swap(m_pid, other.m_pid);
		}
		
		return *this;
	}
	
	
	template <subprocess_handle_spec t_handle_spec>
	auto subprocess <t_handle_spec>::close() -> close_return_type
	{
		std::apply([](auto && ... handle) { ((handle.close()), ...); }, m_handles);
		return m_process.close();
	}
	
	
	template <subprocess_handle_spec t_handle_spec>
	subprocess_status subprocess <t_handle_spec>::open(char const * const path, subprocess_handle_spec const spec)
	{
		char const * const argv[]{path, nullptr};
		return open__(argv, spec);
	}
	
	
	template <subprocess_handle_spec t_handle_spec>
	subprocess_status subprocess <t_handle_spec>::parse_and_open(char const * const args, subprocess_handle_spec const spec)
	{
		auto parsed_args(parse_command_arguments(args));
		return open_(parsed_args, spec);
	}
	
	
	template <subprocess_handle_spec t_handle_spec>
	template <typename t_type>
	subprocess_status subprocess <t_handle_spec>::open_(t_type const &args, subprocess_handle_spec const spec)
	{
		auto const argc(args.size());
		auto argv(static_cast <char const **>(::alloca((argc + 1) * sizeof(char const *))));
		for (auto &&[i, arg] : ranges::views::enumerate(args))
			argv[i] = detail::argument_data(arg);
		argv[argc] = nullptr;
		
		return open__(argv, spec);
	}
	
	
	template <subprocess_handle_spec t_handle_spec>
	subprocess_status subprocess <t_handle_spec>::open__(char const * const argv[], subprocess_handle_spec const spec)
	{
		// argv must be nullptr-terminated.
		// m_handles has at least enough space for the handles specified by spec.
		return detail::open_subprocess(m_process, argv, spec, m_handles.data());
	}
}

#endif

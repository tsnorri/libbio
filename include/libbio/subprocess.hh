/*
 * Copyright (c) 2022-2023 Tuukka Norri
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


namespace libbio::detail {
	void deallocate_subprocess_object_memory(void *ptr, std::size_t const size, std::size_t const alignment);
}


namespace libbio {
	
	struct shared_memory_allocation_error : public std::bad_alloc
	{
		int error{};
		
		shared_memory_allocation_error(int const error_):
			error(error_)
		{
		}
		
		char const *what() const noexcept override { return strerror(error); }
	};
	
	
	template <typename t_type>
	struct subprocess_object_deallocator
	{
		// All objects are placement-new-constructed.
		void operator()(t_type *ptr) noexcept { ptr->~t_type(); detail::deallocate_subprocess_object_memory(ptr, sizeof(t_type), alignof(t_type)); }
	};
	
	
	template <typename t_type>
	using subprocess_object_ptr = std::unique_ptr <t_type, subprocess_object_deallocator <t_type>>;
	
	
	enum class execution_status_type : std::uint8_t
	{
		no_error,
		file_descriptor_handling_failed,
		exec_failed
	};
	
	
	enum class subprocess_handle_spec : std::uint8_t
	{
		NONE	= 0x0,
		STDIN	= 0x1,
		STDOUT	= 0x2,
		STDERR	= 0x4
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
}


namespace libbio { namespace detail {
	
	struct subprocess_status
	{
		execution_status_type	execution_status{};
		std::uint32_t			line{}; // For debugging.
		int						error{};
	};
	
	
	std::tuple <pid_t, int, int, int, subprocess_object_ptr <subprocess_status>>
	open_subprocess(char const * const args[], subprocess_handle_spec const handle_spec); // nullptr-terminated list of arguments.
	
	
	consteval std::size_t handle_count(subprocess_handle_spec const t_spec)
	{
		return bits::count_bits_set(to_underlying(t_spec));
	}
	
	
	// Calculate the rank of curr in all.
	constexpr std::size_t handle_index(subprocess_handle_spec const curr, subprocess_handle_spec const all)
	{
		typedef std::underlying_type_t <subprocess_handle_spec> underlying_t;
		
		// Require that there is a (non-zero) handle and it is in the given set.
		if (!to_underlying(curr & all))
			throw std::invalid_argument("Given handle specification is either zero or not in the set of all specifications");
		
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
	
	
	template <std::size_t t_n, std::size_t... t_i>
	std::array <libbio::file_handle, t_n>
	make_handle_array_(std::array <int, t_n> const &fds, std::index_sequence <t_i...>)
	{
		return {{libbio::file_handle(std::get <t_i>(fds))...}};
	}
	
	
	template <std::size_t t_n, typename t_idxs = std::make_index_sequence <t_n>>
	std::array <libbio::file_handle, t_n>
	make_handle_array(std::array <int, t_n> const &fds)
	{
		return make_handle_array_(fds, t_idxs{});
	}
	
	
	template <subprocess_handle_spec t_spec, std::size_t t_handle_count = handle_count(t_spec)>
	std::array <libbio::file_handle, t_handle_count>
	make_handle_array(std::tuple <pid_t, int, int, int, subprocess_object_ptr <subprocess_status>> const &pid_fd_tuple)
	{
		typedef std::underlying_type_t <subprocess_handle_spec> underlying_t;
		std::array <int, t_handle_count> fds;
		
		// Compile-time check everything.
		if constexpr (to_underlying(subprocess_handle_spec::STDIN & t_spec))
			std::get <handle_index(subprocess_handle_spec::STDIN,  t_spec)>(fds) = std::get <1>(pid_fd_tuple);
		if constexpr (to_underlying(subprocess_handle_spec::STDOUT & t_spec))
			std::get <handle_index(subprocess_handle_spec::STDOUT, t_spec)>(fds) = std::get <2>(pid_fd_tuple);
		if constexpr (to_underlying(subprocess_handle_spec::STDERR & t_spec))
			std::get <handle_index(subprocess_handle_spec::STDERR, t_spec)>(fds) = std::get <3>(pid_fd_tuple);
		
		return make_handle_array(fds);
	}
	
	
	template <typename t_type>
	constexpr inline decltype(auto) argument_data(t_type const &arg) { return std::data(arg); }
	
	constexpr inline auto argument_data(char const *arg) { return arg; }
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
		pid_t pid() const { return m_pid; }
		close_return_type close();
		~process_handle() { if (-1 != m_pid) close(); }
	};
	
	
	std::vector <std::string> parse_command_arguments(char const *args);
	
	
	template <subprocess_handle_spec t_handle_spec>
	class subprocess
	{
	public:
		typedef process_handle::close_return_type	close_return_type;
		typedef std::array <
			file_handle,
			detail::handle_count(t_handle_spec)
		>											handle_array;
		
		typedef detail::subprocess_status			status_type;
		typedef subprocess_object_ptr <status_type>	status_type_ptr;
		
	protected:
		status_type_ptr	m_status{};
		process_handle	m_process{};	// Data member destructors called in reverse order; important for closing the pipe first.
		handle_array	m_handles;
		
	protected:
		explicit subprocess(std::tuple <pid_t, int, int, int, status_type_ptr> &&tup):
			m_status(std::move(std::get <4>(tup))),
			m_process(std::get <0>(tup)),
			m_handles(detail::make_handle_array <t_handle_spec>(tup))
		{
		}
		
		template <typename t_type>
		static subprocess subprocess_with_arguments_(t_type const &args, subprocess_handle_spec const spec);
		
		template <subprocess_handle_spec t_spec>
		constexpr static bool has_handle() { return libbio::has_handle <t_handle_spec, t_spec>(); }
		
		constexpr static std::size_t handle_index(subprocess_handle_spec const spec) { return detail::handle_index(spec, t_handle_spec); }
		template <subprocess_handle_spec t_spec> constexpr file_handle       &handle()       { return std::get <handle_index(t_spec)>(m_handles); }
		template <subprocess_handle_spec t_spec> constexpr file_handle const &handle() const { return std::get <handle_index(t_spec)>(m_handles); }
		
	public:
		subprocess() = default;
		
		// The second argument is useful for having a set of subprocess instances of the same size.
		explicit subprocess(char const * const args[], subprocess_handle_spec const spec = t_handle_spec):
			subprocess(detail::open_subprocess(args, spec))
		{
			libbio_assert_eq(0x0, to_underlying(spec) & ~to_underlying(t_handle_spec));
		}
		
		static subprocess subprocess_with_arguments(std::initializer_list <char const *> const &il, subprocess_handle_spec const spec = t_handle_spec) { return subprocess_with_arguments_(il, spec); }
		static subprocess subprocess_with_arguments(std::vector <std::string> const &args, subprocess_handle_spec const spec = t_handle_spec) { return subprocess_with_arguments_(args, spec); }
		static subprocess subprocess_with_arguments(char const * const args, subprocess_handle_spec const spec = t_handle_spec);
		
		status_type const &status() const { return *m_status; }
		inline close_return_type close();
		
		file_handle       &stdin_handle()        requires (has_handle <subprocess_handle_spec::STDIN>())  { return handle <subprocess_handle_spec::STDIN>(); }
		file_handle const &stdin_handle()  const requires (has_handle <subprocess_handle_spec::STDIN>())  { return handle <subprocess_handle_spec::STDIN>(); }
		file_handle       &stdout_handle()       requires (has_handle <subprocess_handle_spec::STDOUT>()) { return handle <subprocess_handle_spec::STDOUT>(); }
		file_handle const &stdout_handle() const requires (has_handle <subprocess_handle_spec::STDOUT>()) { return handle <subprocess_handle_spec::STDOUT>(); }
		file_handle       &stderr_handle()       requires (has_handle <subprocess_handle_spec::STDERR>()) { return handle <subprocess_handle_spec::STDERR>(); }
		file_handle const &stderr_handle() const requires (has_handle <subprocess_handle_spec::STDERR>()) { return handle <subprocess_handle_spec::STDERR>(); }
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
	template <typename t_type>
	subprocess <t_handle_spec> subprocess <t_handle_spec>::subprocess_with_arguments_(t_type const &args, subprocess_handle_spec const spec)
	{
		auto const argc(args.size());
		auto argv(static_cast <char const *>(::alloca((argc + 1) * sizeof(char const *))));
		for (auto &&[i, arg] : ranges::views::enumerate(args))
			argv[i] = detail::argument_data(arg);
		argv[argc] = nullptr;
		return subprocess <t_handle_spec>(argv, spec);
	}
	
	
	template <subprocess_handle_spec t_handle_spec>
	subprocess <t_handle_spec> subprocess <t_handle_spec>::subprocess_with_arguments(char const * const args, subprocess_handle_spec const spec)
	{
		auto parsed_args(parse_command_arguments(args));
		return subprocess_with_arguments(parsed_args);
	}
}

#endif

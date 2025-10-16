/*
 * Copyright (c) 2022-2025 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_TUPLE_REUSABLE_TUPLE_HH
#define LIBBIO_TUPLE_REUSABLE_TUPLE_HH

#include <cstddef>
#include <libbio/assert.hh>
#include <libbio/tuple/access.hh>
#include <libbio/tuple/fold.hh>
#include <libbio/tuple/for.hh>
#include <libbio/tuple/slice.hh>
#include <libbio/utility/misc.hh>
#include <tuple>
#include <type_traits> // std::bool_constant, std::false_type, std::true_type
#include <utility>


namespace libbio::tuples::detail {

	template <typename t_base_address, typename t_type>
	struct add_to_reusable_tuple_size
	{
		typedef size_constant <
			next_aligned_address_v <
				t_base_address::value,
				alignof(t_type)
			> + sizeof(t_type)
		>	type;
	};

	template <typename t_acc, typename t_type>
	using add_to_reusable_tuple_size_t = typename add_to_reusable_tuple_size <t_acc, t_type>::type;


	template <std::size_t t_idx, typename t_tuple>
	auto const &get_const_reference(t_tuple const &tuple)
	{
		typedef std::tuple_element_t <t_idx, t_tuple>							element_type;
		typedef std::add_lvalue_reference_t <std::add_const_t <element_type>>	return_type;
		return static_cast <return_type>(std::get <t_idx>(tuple));
	}


	template <typename t_type>
	static void cast_and_clear(void *ptr)
	{
		reinterpret_cast <t_type *>(ptr)->clear();
	}

	template <typename t_type>
	struct cast_and_clear_fn : public std::integral_constant <void (*)(void *), nullptr> {};

	template <typename t_type>
	constexpr inline auto const cast_and_clear_fn_v{cast_and_clear_fn <t_type>::value};
}


namespace libbio::tuples {

	template <typename>
	struct reusable_tuple_size {};

	template <typename... t_types>
	struct reusable_tuple_size <std::tuple <t_types...>> : public std::integral_constant <
		std::size_t,
		foldl_t <detail::add_to_reusable_tuple_size_t, size_constant <0>, std::tuple <t_types...>>::value
	>
	{
	};

	// Specialisation for reusable_tuple after the class definition.

	template <typename t_tuple>
	constexpr static inline auto const reusable_tuple_size_v{reusable_tuple_size <t_tuple>::value};


	// Reusable in the sense that the memory is not deallocated. Appended types are constructed in place without moving.
	// FIXME: std::launder likely needs to be applied.
	template <std::size_t t_max_size, std::size_t t_alignment, typename... t_args>
	class reusable_tuple
	{
	public:
		typedef std::tuple <t_args...>						tuple_type;
		typedef reusable_tuple <t_max_size, t_alignment>	empty_type;
		constexpr static inline std::size_t const tuple_size{sizeof...(t_args)};

	private:
		template <typename t_tuple>
		constexpr static inline auto const offset_of_last_v{reusable_tuple_size_v <t_tuple> - sizeof(last_t <t_tuple>)};

		template <std::size_t t_idx>
		constexpr static inline auto const offset_of_v{offset_of_last_v <slice_t <tuple_type, 0, 1 + t_idx>>};

	private:
		void							(*m_clear_fn)(void *){};
		alignas(t_alignment) std::byte	m_buffer[t_max_size]{};

	public:
		template <typename t_type>
		using append_t = reusable_tuple <t_max_size, t_alignment, t_args..., t_type>;

		template <typename t_type>
		struct can_fit : std::false_type {};

		template <std::size_t t_max_size_, std::size_t t_alignment_, typename... t_args_>
		struct can_fit <reusable_tuple <t_max_size_, t_alignment_, t_args_...>> : public std::bool_constant <
			t_max_size_ <= t_max_size && t_alignment_ <= t_alignment && 0 == t_alignment % t_alignment_
		> {};

		template <typename t_type>
		constexpr static bool const can_fit_v{can_fit <t_type>::value}; // For some reason Clang is not able to deduce the type if this is declared as auto instead of bool.

	private:
		template <
			typename t_tuple,
			typename t_self,
			typename t_return = add_const_if_t <
				last_t <t_tuple>,
				std::is_const_v <std::remove_reference_t <t_self>>
			>
		>
		constexpr static t_return *address_of_last(t_self &&self)
		{
			constexpr auto const offset(offset_of_last_v <t_tuple>);
			static_assert(0 == offset % alignof(t_return));
			auto * const addr(std::addressof(self.m_buffer[0]) + offset);
			return reinterpret_cast <t_return *>(addr);
		}

		template <std::size_t t_idx, typename t_self>
		constexpr static auto *address_of(t_self &&self)
		{
			return address_of_last <slice_t <tuple_type, 0, 1 + t_idx>>(self);
		}

	public:
		constexpr empty_type &clear()
		{
			libbio_assert_eq(m_clear_fn, detail::cast_and_clear_fn_v <reusable_tuple>, "clear() called for mismatching type");
			for_ <tuple_size>([this]<typename t_idx>(){
				constexpr auto const idx(t_idx::value);
				typedef std::tuple_element_t <idx, tuple_type> current_type;
				if constexpr (!std::is_trivially_destructible_v <current_type>)
				{
					auto * const addr(address_of <idx>(*this));
					addr->~current_type();
				}
			});
			m_clear_fn = nullptr;
			return reinterpret_cast <empty_type &>(*this);
		}

		constexpr ~reusable_tuple()
		{
			if (m_clear_fn)
				(*m_clear_fn)(this);
		}

		template <typename t_type, typename... t_args_>
		constexpr append_t <t_type> &append(t_args_ && ... args)
		{
			typedef std::tuple <t_args..., t_type> new_tuple_type;
			static_assert(reusable_tuple_size_v <new_tuple_type> <= t_max_size, "Adding the type exceeds the tuple size");
			new (address_of_last <new_tuple_type>(*this)) t_type(std::forward <t_args_>(args)...);
			m_clear_fn = detail::cast_and_clear_fn_v <append_t <t_type>>;
			return reinterpret_cast <append_t <t_type> &>(*this);
		}

		template <std::size_t t_idx>
		constexpr auto const &get() const
		{
			return *address_of <t_idx>(*this);
		}

		template <std::size_t t_idx>
		constexpr auto &get()
		{
			return *address_of <t_idx>(*this);
		}

		constexpr auto const &back() const
		{
			static_assert(0 < tuple_size);
			return get <tuple_size - 1>();
		}

		constexpr auto &back()
		{
			static_assert(0 < tuple_size);
			return get <tuple_size - 1>();
		}

		template <typename... t_args_>
		constexpr bool operator==(std::tuple <t_args_...> const &tuple) const
		{
			if constexpr (std::is_same_v <tuple_type, std::tuple <t_args_...>>)
			{
				return [this, &tuple]<std::size_t... t_idxs>(std::index_sequence <t_idxs...>) constexpr {
					return ((get <t_idxs>() == detail::get_const_reference <t_idxs>(tuple)) && ...);
				}(std::make_index_sequence <tuple_size>{});
			}
			else
			{
				// Type mismatch.
				return false;
			}
		}
	};


	template <typename... t_lhs_args, std::size_t t_max_size, std::size_t t_alignment, typename... t_rhs_args>
	constexpr bool operator==(std::tuple <t_lhs_args...> const &lhs, reusable_tuple <t_max_size, t_alignment, t_rhs_args...> const &rhs)
	{
		return rhs.template operator==(lhs);
	}


	template <std::size_t t_max_size, std::size_t t_alignment, typename... t_args>
	struct reusable_tuple_size <reusable_tuple <t_max_size, t_alignment, t_args...>> : public std::integral_constant <
		std::size_t,
		foldl_t <
			detail::add_to_reusable_tuple_size_t,
			size_constant <0>,
			typename reusable_tuple <t_max_size, t_alignment, t_args...>::tuple_type
		>::value
	> {};


	template <typename t_tuple>
	struct is_reusable_tuple : public std::false_type {};

	template <std::size_t t_max_size, std::size_t t_alignment, typename... t_args>
	struct is_reusable_tuple <reusable_tuple <t_max_size, t_alignment, t_args...>> : public std::true_type {};

	template <typename t_tuple>
	constexpr inline auto const is_reusable_tuple_v{is_reusable_tuple <t_tuple>::value};


	template <std::size_t t_idx, typename t_tuple>
	constexpr auto &get(t_tuple &&tuple)
	requires is_reusable_tuple_v <std::remove_cvref_t <t_tuple>>
	{
		return tuple.template get <t_idx>();
	}
}


namespace libbio::tuples::detail {

	template <std::size_t t_max_size, std::size_t t_alignment, typename... t_args>
	struct cast_and_clear_fn <reusable_tuple <t_max_size, t_alignment, t_args...>> :
		public std::integral_constant <void (*)(void *),
			0 == sizeof...(t_args) ? nullptr : &cast_and_clear <reusable_tuple <t_max_size, t_alignment, t_args...>>
		> {};
}


// std::tuple_size and std::tuple_element are customisation points in the standard library.
template <std::size_t t_max_size, std::size_t t_alignment, typename... t_args>
struct std::tuple_size <libbio::tuples::reusable_tuple <t_max_size, t_alignment, t_args...>> :
	public std::integral_constant <std::size_t, sizeof...(t_args)> {};

template <std::size_t t_idx, std::size_t t_max_size, std::size_t t_alignment, typename... t_args>
struct std::tuple_element <t_idx, libbio::tuples::reusable_tuple <t_max_size, t_alignment, t_args...>> :
	public std::tuple_element <t_idx, typename libbio::tuples::reusable_tuple <t_max_size, t_alignment, t_args...>::tuple_type> {};

#endif

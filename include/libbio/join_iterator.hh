/*
 * Copyright (c) 2026 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_JOIN_ITERATOR_HH
#define LIBBIO_JOIN_ITERATOR_HH

#include <array>
#include <cstddef>
#include <functional>
#include <iterator>
#include <span>
#include <type_traits>


namespace libbio::detail {

	template <bool t_should_dereference /* = true */>
	struct conditional_dereference
	{
		template <typename t_value>
		decltype(auto) operator()(t_value &&value) { return *value; }
	};

	template <>
	struct conditional_dereference <false> : public std::identity {};
}


namespace libbio {

	// LegacyForwardIterator for joining the values of a range of containers.
	template <typename t_container_iterator, typename t_container_sentinel, bool t_uses_indirection>
	class join_iterator_
	{
	public:
		typedef t_container_iterator container_iterator;
		typedef t_container_sentinel container_sentinel;
		typedef std::remove_pointer_t <std::iter_value_t <container_iterator>> container_type; // Remove pointer in case of indirection.
		typedef std::conditional_t <
			std::is_const_v <container_type>,
			typename container_type::const_iterator,
			typename container_type::iterator
		> underlying_iterator;
		typedef std::iter_value_t <underlying_iterator> underlying_value_type;

	private:
		typedef detail::conditional_dereference <t_uses_indirection> access_container_type;

		container_iterator m_cit{};
		container_iterator m_cend{};
		underlying_iterator m_it{};

	public:
		typedef std::forward_iterator_tag iterator_category;
		typedef std::ptrdiff_t difference_type;
		typedef std::iter_value_t <underlying_iterator> value_type;
		typedef std::iter_reference_t <underlying_iterator> reference;
		typedef std::iterator_traits <underlying_iterator>::pointer pointer;

		constexpr join_iterator_() = default;

		constexpr explicit join_iterator_(container_iterator it, container_sentinel end):
			m_cit{it},
			m_cend{end},
			m_it{
				(m_cit == m_cend)
				? underlying_iterator{}
				: access_container_type{}(*m_cit).begin()
			}
		{
		}

		constexpr reference operator*() const
		{
			return *m_it;
		}

		constexpr pointer operator->() const
		{
			return &*m_it;
		}

		constexpr join_iterator_ &operator++()
		{
			++m_it;
			access_container_type acc{};
			if (m_it == acc(*m_cit).end())
			{
				++m_cit;
				if (m_cit != m_cend)
					m_it = acc(*m_cit).begin();
			}
		}

		constexpr join_iterator_ operator++(int)
		{
			join_iterator_ retval{*this};
			++(*this);
			return retval;
		}

		friend constexpr bool operator==(join_iterator_ const &lhs, join_iterator_ const &rhs)
		{
			return lhs.m_it == rhs.m_it;
		}

		friend constexpr bool operator!=(join_iterator_ const &lhs, join_iterator_ const &rhs)
		{
			return !(lhs == rhs);
		}
	};


	template <typename t_container_iterator, typename t_container_sentinel = t_container_iterator>
	using join_iterator = join_iterator_ <t_container_iterator, t_container_sentinel, false>;

	template <typename t_container_iterator, typename t_container_sentinel = t_container_iterator>
	using indirect_join_iterator = join_iterator_ <t_container_iterator, t_container_sentinel, true>;


	template <typename t_iterator, typename t_sentinel, std::size_t t_size>
	requires(t_size != std::dynamic_extent) // FIXME: Specialise for std::dynamic_extent
	class joined_range_iterator
	{
	private:
		typedef std::array <t_iterator, t_size> iterator_array;
		typedef std::array <t_sentinel, t_size> sentinel_array;

	private:
		iterator_array m_iterators{};
		sentinel_array m_sentinels{};
		std::size_t m_pos{};

	public:
		typedef std::forward_iterator_tag iterator_category;
		typedef std::ptrdiff_t difference_type;
		typedef std::iter_value_t <t_iterator> value_type;
		typedef std::iter_reference_t <t_iterator> reference;
		typedef std::iterator_traits <t_iterator>::pointer pointer;

	public:
		constexpr joined_range_iterator() = default;

		constexpr joined_range_iterator(iterator_array &&iterators, sentinel_array &&sentinels, std::size_t pos = 0):
			m_iterators{std::move(iterators)},
			m_sentinels{std::move(sentinels)},
			m_pos{pos}
		{
		}

		constexpr static joined_range_iterator make_sentinel() { return {iterator_array{}, sentinel_array{}, t_size}; }

		constexpr reference operator*() const
		{
			return *m_iterators[m_pos];
		}

		constexpr pointer operator->() const
		{
			return &*m_iterators[m_pos];
		}

		constexpr joined_range_iterator &operator++()
		{
			++m_iterators[m_pos];
			while (m_pos < t_size && m_iterators[m_pos] == m_sentinels[m_pos])
				++m_pos;
		}

		constexpr joined_range_iterator operator++(int)
		{
			joined_range_iterator retval{*this};
			++(*this);
			return retval;
		}

		friend constexpr bool operator==(joined_range_iterator const &lhs, joined_range_iterator const &rhs)
		{
			// This would be way simpler had we sentinel elementsin in the arrays.
			if (lhs.m_pos == t_size)
			{
				if (rhs.m_pos == t_size)
					return true;
				return false;
			}
			else if (rhs.m_pos == t_size)
			{
				return false;
			}

			return lhs.m_iterators[lhs.m_pos] == rhs.m_iterators[rhs.m_pos];
		}

		friend constexpr bool operator!=(joined_range_iterator const &lhs, joined_range_iterator const &rhs)
		{
			return !(lhs == rhs);
		}
	};

	template <typename t_iterator, typename t_sentinel, std::size_t t_size>
	joined_range_iterator(
		std::array <t_iterator, t_size> &&,
		std::array <t_sentinel, t_size> &&
	) -> joined_range_iterator <t_iterator, t_sentinel, t_size>;
}

#endif

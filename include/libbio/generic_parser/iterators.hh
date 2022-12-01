/*
 * Copyright (c) 2022 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_GENERIC_PARSER_ITERATOR_HH
#define LIBBIO_GENERIC_PARSER_ITERATOR_HH

#include <boost/iterator/iterator_adaptor.hpp>
#include <iterator>								// std::distance, std::contiguous_iterator
#include <libbio/generic_parser/fwd.hh>


namespace libbio::parsing::detail {

#if 0
	// Fwd.
	template <typename> struct distance_;
#endif
	template <bool> struct range_position;
	
	
	// Maintain the numeric position while iterating.
	template <typename t_iterator>
	class counting_iterator : public boost::iterator_adaptor <counting_iterator <t_iterator>, t_iterator>
	{
		friend class boost::iterator_core_access;
		
		template <bool> friend struct range_position;
		
#if 0
		template <typename> friend struct distance_;
#endif
		template <typename, bool, typename...> friend class parser;
		
	public:
		typedef boost::iterator_adaptor <counting_iterator <t_iterator>, t_iterator>	iterator_adaptor_;
		typedef typename iterator_adaptor_::difference_type								difference_type;
		
	private:
		std::size_t pos{};
		
	public:
		counting_iterator():
			iterator_adaptor_(t_iterator{})
		{
		}
		
		explicit counting_iterator(t_iterator it):
			iterator_adaptor_(it)
		{
		}
		
	private:
		void increment()
		{
			++(iterator_adaptor_::base_reference());
			++pos;
		}
		
		void decrement()
		{
			--(iterator_adaptor_::base_reference());
			--pos;
		}
		
		void advance(difference_type const n)
		{
			iterator_adaptor_::base_reference() += n;
			pos += n;
		}
	};
	
	
#if 0
	template <typename t_iterator>
	struct distance_
	{
		std::size_t operator()(t_iterator const lhs, t_iterator const rhs) const
		{
			return std::distance(lhs, rhs);
		}
	};
	
	
	template <typename t_iterator>
	struct distance_ <counting_iterator <t_iterator>>
	{
		std::size_t operator()(counting_iterator <t_iterator> const, counting_iterator <t_iterator> const rhs) const
		{
			return rhs.pos;
		}
	};
	
	
	template <typename t_iterator>
	std::size_t distance(t_iterator const lhs, t_iterator const rhs)
	{
		distance_ <t_iterator> dd;
		return dd(lhs, rhs);
	}
#endif
	
	
	// Fwd.
	template <typename t_iterator, typename t_sentinel, typename t_callback>
	struct updatable_range;
	
	
	template <typename t_iterator, typename t_sentinel, typename t_callback>
	class updatable_range_iterator : public boost::iterator_adaptor <
		updatable_range_iterator <t_iterator, t_sentinel, t_callback>,
		t_iterator,
		boost::use_default,
		boost::forward_traversal_tag
	>
	{
		// Self-contained b.c. we need to know when update() should be called.
		friend class boost::iterator_core_access;
		
	public:
		typedef boost::iterator_adaptor <
			updatable_range_iterator <t_iterator, t_sentinel, t_callback>,
			t_iterator,
			boost::use_default,
			boost::forward_traversal_tag
		>																				iterator_adaptor_;
		typedef typename iterator_adaptor_::difference_type								difference_type;
		typedef updatable_range <t_iterator, t_sentinel, t_callback>					updatable_range_type;
		
	private:
		updatable_range_type	*m_range{};
		
	public:
		updatable_range_iterator() = default;
		
		explicit updatable_range_iterator(updatable_range_type &range):
			m_range(&range)
		{
		}
		
		void increment()
		{
			++m_range->it;
			if (m_range->it == m_range->sentinel)
				m_range->update();
		}
		
		bool equal(updatable_range_iterator const &) const { return m_range->it == m_range->sentinel; }
	};
	
	
	template <bool t_is_contiguous>
	struct range_position
	{
		template <typename t_iterator, typename t_sentinel>
		range_position(t_iterator &, t_sentinel const) // No-op.
		{
		}
		
		template <typename t_iterator, typename t_sentinel>
		std::size_t distance(t_iterator &it, t_sentinel const) const { return it.pos; }
	};
	
	
	template <>
	struct range_position <true>
	{
		std::size_t value{};
		
		template <typename t_iterator, typename t_sentinel>
		range_position(t_iterator &it, t_sentinel const sentinel):
			value(std::distance(it, sentinel))
		{
		}
		
		template <typename t_iterator, typename t_sentinel>
		std::size_t distance(t_iterator &it, t_sentinel const sentinel) const { return value - std::distance(it, sentinel); }
	};
	
	
	template <typename t_iterator, typename t_sentinel>
	struct range
	{
		typedef t_iterator	iterator;
		constexpr static inline bool is_contiguous{std::contiguous_iterator <iterator>};
		constexpr static inline bool is_updatable{false};
		
		t_iterator &it;
		t_sentinel const sentinel;
		range_position <is_contiguous> pos;
		
		range(t_iterator &it_, t_sentinel const sentinel_):
			it(it_),
			sentinel(sentinel_),
			pos(it, sentinel)
		{
		}
		
		bool is_at_end() const { return it == sentinel; }
		auto joining_iterator_pair() { return std::tie(it, sentinel); }
		std::size_t position() const { return pos.distance(it, sentinel); }
	};
	
	
	template <typename t_iterator, typename t_sentinel, typename t_callback>
	struct updatable_range
	{
		typedef t_iterator														iterator;
		typedef updatable_range_iterator <t_iterator, t_sentinel, t_callback>	updatable_iterator;
		
		constexpr static inline bool is_contiguous{std::contiguous_iterator <iterator>};
		constexpr static inline bool is_updatable{true};
		
		t_iterator	&it;
		t_sentinel	&sentinel;
		t_callback	&update_callback;
		std::size_t	cumulative_length{};
		
		updatable_range(t_iterator &it_, t_sentinel &sentinel_, t_callback &update_callback_):
			it(it_),
			sentinel(sentinel_),
			update_callback(update_callback_)
		{
		}
		
		bool is_at_end() { return it == sentinel && (!update() || it == sentinel); }
		auto joining_iterator_pair() { return std::make_tuple(updatable_iterator(*this), updatable_iterator()); }
		
		inline bool update()
		{
			// To save a bit of space, we have a special case for contiguous iterators.
			if constexpr (is_contiguous)
			{
				auto const retval(update_callback(it, sentinel));
				cumulative_length += std::distance(it, sentinel); // Adds zero if updating fails.
				return retval;
			}
			else
			{
				cumulative_length += it.pos;
				return update_callback(it, sentinel);
			}
		}
		
		std::size_t position() const
		{
			if constexpr (is_contiguous)
				return cumulative_length - std::distance(it, sentinel);
			else
				return cumulative_length + it.pos;
		}
	};
}

#endif

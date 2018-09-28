/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_ARRAY_LIST_HH
#define LIBBIO_ARRAY_LIST_HH

#include <boost/iterator/iterator_facade.hpp>
#include <boost/serialization/utility.hpp>
#include <iostream>
#include <libbio/util.hh>
#include <type_traits>
#include <vector>


namespace libbio {
	template <typename t_item> class array_list;
}

// Forward declaration needed for template friends.
namespace boost { namespace serialization {
	
	template <typename t_archive, typename t_value>
	void load(t_archive &, libbio::array_list <t_value> &, unsigned int const);
	
	template <typename t_archive, typename t_value>
	void save(t_archive &, libbio::array_list <t_value> const &, unsigned int const);
}}


namespace libbio { namespace detail {
	
	template <typename t_value> class array_list_item_iterator;
	template <typename t_value> class array_list_pair_iterator;
	template <typename t_value> class array_list_value_iterator;
	
	template <typename t_value>
	struct array_list_item;
	
	
	template <typename t_cls, typename t_list, typename t_reference>
	class array_list_iterator_base : public boost::iterator_facade <
		t_cls,
		std::remove_reference_t <t_reference>,
		boost::bidirectional_traversal_tag,
		t_reference
	>
	{
		template <typename, typename, typename>
		friend class array_list_iterator_base;
		
		friend class boost::iterator_core_access;
		friend t_list;
		
	protected:
		typedef t_list								array_list_type;
		typedef typename t_list::item_type			item_type;
		typedef array_list_value_iterator <t_list>	value_iterator_type;
		typedef array_list_pair_iterator <t_list>	pair_iterator_type;
		
	protected:
		array_list_type						*m_list{nullptr};
		typename array_list_type::size_type	m_idx{0};
		
	public:
		array_list_iterator_base() = default;
		
		array_list_iterator_base(array_list_type &list, typename array_list_type::size_type const idx):
			m_list(&list),
			m_idx(idx)
		{
		}
		
		template <
			typename t_other_cls,
			typename t_other_list,
			typename t_other_reference,
			typename = std::enable_if <
				std::is_convertible <typename t_list::value_type *, typename t_other_list::value_type *>::value
			>
		>
		array_list_iterator_base(array_list_iterator_base <t_other_cls, t_other_list, t_other_reference> const &it):
			m_list(it.m_list),
			m_idx(it.m_idx)
		{
		}
		
	protected:
		void increment() { m_idx = m_list->item(m_idx).next; }
		void decrement() { m_idx = (SIZE_MAX == m_idx ? m_list->last_index() : m_list->item(m_idx).prev); }
		bool equal(array_list_iterator_base const &other) const { return m_list == other.m_list && m_idx == other.m_idx; }
		
		item_type &item() { return m_list->item(m_idx); }
		item_type const &item() const { return m_list->item(m_idx); }
		
		value_iterator_type value_iterator() const { return value_iterator_type(*m_list, m_idx); }
		pair_iterator_type pair_iterator() const { return pair_iterator_type(*m_list, m_idx); }
	};
	
	
	template <typename t_list>
	struct array_list_value_iterator_detail
	{
		// Make value_type const if t_list is const.
		typedef make_const_t <typename t_list::value_type, std::is_const_v <t_list>>	value_type;
		static_assert(std::is_const_v <t_list> == std::is_const_v <value_type>);
		
		typedef array_list_iterator_base <
			array_list_value_iterator <t_list>,
			t_list,
			value_type &
		> base_class;
	};
	
	template <typename t_list>
	class array_list_value_iterator final : public array_list_value_iterator_detail <t_list>::base_class
	{
		friend class boost::iterator_core_access;
		
	protected:
		typedef array_list_value_iterator_detail <t_list>	detail_type;
		typedef typename detail_type::value_type			value_type;
		typedef typename detail_type::base_class			base_class;
		
	public:
		using base_class::base_class;
		value_type &dereference() const { return this->m_list->item(this->m_idx).value; }
	};
	
	
	template <typename t_list>
	struct array_list_item_iterator_detail
	{
		typedef array_list_item <t_list>	item_type;
		typedef typename t_list::value_type	value_type;
		
		typedef array_list_iterator_base <
			array_list_item_iterator <t_list>,
			t_list,
			item_type &
		> base_class;
	};
	
	template <typename t_list>
	class array_list_item_iterator final : public array_list_item_iterator_detail <t_list>::base_class
	{
		friend class boost::iterator_core_access;
	
	protected:
		typedef array_list_item_iterator_detail <t_list>	detail_type;
		typedef typename detail_type::item_type				item_type;
		typedef typename detail_type::item_type				value_type;
		typedef typename detail_type::base_class			base_class;
	
	public:
		using base_class::base_class;
		item_type &dereference() const { return this->item(); }
	};
	
	
	template <typename t_list>
	struct array_list_pair_iterator_detail
	{
		// Make value_type const if t_list is const.
		typedef make_const_t <typename t_list::value_type, std::is_const_v <t_list>>	value_type;
		static_assert(std::is_const_v <t_list> == std::is_const_v <value_type>);
		typedef std::pair <typename array_list <t_list>::size_type, value_type &>		pair_type;
		
		typedef array_list_iterator_base <
			array_list_pair_iterator <t_list>,
			t_list,
			pair_type
		> base_class;
	};
	
	template <typename t_list>
	class array_list_pair_iterator final : public array_list_pair_iterator_detail <t_list>::base_class
	{
		friend class boost::iterator_core_access;
		
	protected:
		typedef array_list_pair_iterator_detail <t_list>	detail_type;
		typedef typename detail_type::base_class			base_class;
		typedef typename detail_type::pair_type				pair_type;
		
	public:
		using base_class::base_class;
		pair_type dereference() const { return pair_type(this->m_idx, this->item().value); }
	};
	
	
	template <typename t_value>
	struct array_list_item
	{
		typedef array_list <t_value>	array_list_type;
		
		std::size_t						prev{SIZE_MAX};
		std::size_t						next{SIZE_MAX};
		t_value							value;
		
		explicit array_list_item(t_value const &value_ = t_value()): value(value_) {}
		array_list_item(t_value &&value_): value(std::move(value_)) {}
		
		bool const has_prev() const { return SIZE_MAX != prev; }
		bool const has_next() const { return SIZE_MAX != next; }
	};
	
	template <typename t_value>
	std::ostream &operator<<(std::ostream &stream, array_list_item <t_value> const &item)
	{
		stream << "prev: " << item.prev << " next: " << item.next << " value: " << item.value;
		return stream;
	}
	
	
	template <typename t_array_list>
	class array_list_item_iterator_proxy
	{
	public:
		typedef std::conditional_t <
			std::is_const_v <t_array_list>,
			typename t_array_list::const_item_iterator,
			typename t_array_list::item_iterator
		>													iterator;
		typedef typename t_array_list::const_item_iterator	const_iterator;
		
	protected:
		t_array_list	*m_array_list{nullptr};
		
	public:
		array_list_item_iterator_proxy() = default;
		array_list_item_iterator_proxy(t_array_list &array_list): m_array_list(*array_list) {}
		
		iterator begin()				{ return m_array_list->begin_items(); }
		iterator end()					{ return m_array_list->end_items(); }
		
		const_iterator begin() const	{ return m_array_list->begin_items(); }
		const_iterator end() const		{ return m_array_list->end_items(); }
	};
	
	
	template <typename t_array_list>
	class array_list_pair_iterator_proxy
	{
	public:
		typedef std::conditional_t <
			std::is_const_v <t_array_list>,
			typename t_array_list::const_pair_iterator,
			typename t_array_list::pair_iterator
		>													iterator;
		typedef typename t_array_list::const_pair_iterator	const_iterator;
		
	protected:
		t_array_list	*m_array_list{nullptr};
		
	public:
		array_list_pair_iterator_proxy() = default;
		array_list_pair_iterator_proxy(t_array_list &array_list): m_array_list(&array_list) {}
		
		iterator begin()				{ return m_array_list->begin_pairs(); }
		iterator end()					{ return m_array_list->end_pairs(); }
		
		const_iterator begin() const	{ return m_array_list->begin_pairs(); }
		const_iterator end() const		{ return m_array_list->end_pairs(); }
	};
}}


namespace libbio {
	
	// A doubly-linked list backed by an std::vector.
	// Should provide more memory locality than a pointer-based list.
	template <typename t_value>
	class array_list
	{
		template <typename t_archive, typename t_tpl_value>
		friend void boost::serialization::save(t_archive &, array_list <t_tpl_value> const &, unsigned int const);
		
		template <typename t_archive, typename t_tpl_value>
		friend void boost::serialization::load(t_archive &, array_list <t_tpl_value> &, unsigned int const);
		
	public:
		typedef detail::array_list_item <t_value>									item_type;
		
	protected:
		typedef std::vector <item_type>												container_type;
		typedef array_list <t_value>												array_list_type;
		
	public:
		typedef t_value																value_type;
		typedef typename container_type::size_type									size_type;
		typedef value_type															&reference;
		typedef value_type															&const_reference;
		typedef detail::array_list_value_iterator <array_list_type>					iterator;
		typedef detail::array_list_value_iterator <array_list_type const>			const_iterator;
		typedef detail::array_list_item_iterator <array_list_type>					item_iterator;
		typedef detail::array_list_item_iterator <array_list_type const>			const_item_iterator;
		typedef detail::array_list_pair_iterator <array_list_type>					pair_iterator;
		typedef detail::array_list_pair_iterator <array_list_type const>			const_pair_iterator;
		
		typedef detail::array_list_item_iterator_proxy <array_list_type>			item_iterator_proxy_type;
		typedef detail::array_list_item_iterator_proxy <array_list_type const>		const_item_iterator_proxy_type;
		typedef detail::array_list_pair_iterator_proxy <array_list_type>			pair_iterator_proxy_type;
		typedef detail::array_list_pair_iterator_proxy <array_list_type const>		const_pair_iterator_proxy_type;
		
	protected:
		container_type	m_items;
		size_type		m_first{SIZE_MAX};
		size_type		m_last_1{0};
		
	public:
		array_list() = default;
		array_list(array_list &&) = default;
		array_list(array_list const &other) { copy(other); }
		explicit array_list(
			std::initializer_list <std::pair <size_type, t_value>> list
		);
		
		array_list &operator=(array_list &&other) & = default;
		array_list &operator=(array_list const &other) { copy(other); return *this; }
		
		void reset() { m_first = SIZE_MAX; m_last_1 = 0; }
		void set_first_element(size_type first) { m_first = first; }
		void set_last_element(size_type last) { m_last_1 = 1 + last; }
		
		size_type const size() const { return m_items.size(); }
		size_type const first_index() const { return m_first; }
		size_type const last_index() const { return m_last_1 - 1; }	// Valid if 0 < size().
		size_type const last_index_1() const { return m_last_1; }
		
		reference back() { return m_items[m_last_1 - 1].value; }
		const_reference back() const { return m_items[m_last_1 - 1].value; }
		
		reference operator[](size_type const idx) { return m_items[idx].value; }
		const_reference operator[](size_type const idx) const { return m_items[idx].value; }
		reference at(size_type const idx) { return m_items.at(idx).value; }
		const_reference at(size_type const idx) const { return m_items.at(idx).value; }
		
		item_type &item(size_type const idx) { return m_items[idx]; }
		item_type const &item(size_type const idx) const { return m_items[idx]; }
		
		void clear(bool release_memory = false) { (release_memory ? clear_and_resize_vector(m_items) : m_items.clear()); reset(); }
		void resize(size_type const size) { m_items.resize(size); }
		void resize(size_type const size, value_type const &value) { m_items.resize(size, value); }
		void reserve(size_type const capacity) { m_items.reserve(capacity); }
		
		iterator find(size_type const idx) { return iterator(*this, idx); }
		const_iterator find(size_type const idx) const { return const_iterator(*this, idx); }
		
		void erase(iterator pos, bool const change_size = true);
		
		void push_back(t_value const &value);
		void push_back(t_value &&value);
		
		// Assign an item to the given position and update links.
		void link(t_value const &value, size_type const idx, size_type const prev_idx = SIZE_MAX, size_type const next_idx = SIZE_MAX);
		void link(t_value &&value, size_type const idx, size_type const prev_idx = SIZE_MAX, size_type const next_idx = SIZE_MAX);
		
		size_type const prev_idx(size_type const idx) const { return m_items[idx].prev; }
		size_type const next_idx(size_type const idx) const { return m_items[idx].next; }
		
		iterator begin() noexcept { return iterator(*this, m_first); }
		const_iterator begin() const noexcept { return const_iterator(*this, m_first); }
		const_iterator cbegin() const noexcept { return const_iterator(*this, m_first); };
		
		iterator end() noexcept { return iterator(*this, SIZE_MAX); }
		const_iterator end() const noexcept { return const_iterator(*this, SIZE_MAX); }
		const_iterator cend() const noexcept { return const_iterator(*this, SIZE_MAX); }
		
		item_iterator begin_items() noexcept { return item_iterator(*this, m_first); }
		const_item_iterator begin_items() const noexcept { return const_item_iterator(*this, m_first); }
		const_item_iterator cbegin_items() const noexcept { return const_item_iterator(*this, m_first); };
		
		item_iterator end_items() noexcept { return item_iterator(*this, SIZE_MAX); }
		const_item_iterator end_items() const noexcept { return const_item_iterator(*this, SIZE_MAX); }
		const_item_iterator cend_items() const noexcept { return const_item_iterator(*this, SIZE_MAX); }
		
		pair_iterator begin_pairs() noexcept { return pair_iterator(*this, m_first); }
		const_pair_iterator begin_pairs() const noexcept { return const_pair_iterator(*this, m_first); }
		const_pair_iterator cbegin_pairs() const noexcept { return const_pair_iterator(*this, m_first); };
		
		pair_iterator end_pairs() noexcept { return pair_iterator(*this, SIZE_MAX); }
		const_pair_iterator end_pairs() const noexcept { return const_pair_iterator(*this, SIZE_MAX); }
		const_pair_iterator cend_pairs() const noexcept { return const_pair_iterator(*this, SIZE_MAX); }
		
		item_iterator_proxy_type item_iterator_proxy() { return item_iterator_proxy_type(*this); }
		const_item_iterator_proxy_type item_iterator_proxy() const { return const_item_iterator_proxy_type(*this); }
		const_item_iterator_proxy_type const_item_iterator_proxy() const { return const_item_iterator_proxy_type(*this); }
		
		pair_iterator_proxy_type pair_iterator_proxy() { return pair_iterator_proxy_type(*this); }
		const_pair_iterator_proxy_type pair_iterator_proxy() const { return const_pair_iterator_proxy_type(*this); }
		const_pair_iterator_proxy_type const_pair_iterator_proxy() const { return const_pair_iterator_proxy_type(*this); }
		
	protected:
		void copy(array_list const &other);

		void add_item(item_type &&item);
		void link_item(item_type &&item, size_type const idx);
		
		template <typename t_archive>
		void boost_load(t_archive &ar, unsigned int const version);
	
		template <typename t_archive>
		void boost_save(t_archive &ar, unsigned int const version) const;
	};
	
	
	template <typename t_value>
	std::ostream &operator<<(std::ostream &os, array_list <t_value> const &list)
	{
		bool first(true);
		os << '[' << list.first_index() << ", " << list.last_index_1() << ") ";
		for (auto const &kv : list.const_pair_iterator_proxy())
		{
			if (!first)
				os << ' ';
			os << kv.first << ": " << kv.second;
			first = false;
		}
		return os;
	}
	
	
	template <typename t_value>
	array_list <t_value>::array_list(
		std::initializer_list <std::pair <size_type, t_value>> list
	)
	{
		if (list.size())
		{
			auto begin(std::begin(list));
			auto rbegin(std::rbegin(list));
			m_first = begin->first;
			m_last_1 = rbegin->first;
			m_items.resize(1 + m_last_1);
			
			size_type prev_idx(SIZE_MAX);
			for (auto const &pair : list)
			{
				link(pair.second, pair.first, prev_idx);
				prev_idx = pair.first;
			}
		}
	}
	
	
	template <typename t_value>
	void array_list <t_value>::copy(array_list const &other)
	{
		m_items.resize(other.m_items.size());
		m_first = other.m_first;
		m_last_1 = other.m_last_1;

		// Try to save time by copying only the items that are set.
		auto idx(m_first);
		while (SIZE_MAX != idx)
		{
			auto const &item(other.m_items[idx]);
			m_items[idx] = item;
			idx = item.next;
		}
	}


	template <typename t_value>
	void array_list <t_value>::erase(iterator it, bool const change_size)
	{
		assert(end() != it);
		
		auto &item(it.item());
		if (item.has_prev())
		{
			auto prev_it(it);
			--prev_it;
			
			auto &prev_item(prev_it.item());
			prev_item.next = item.next;
		}
		else
		{
			// item.next may be invalid.
			m_first = item.next;
		}
		
		if (item.has_next())
		{
			auto next_it(it);
			++next_it;
			
			auto &next_item(next_it.item());
			next_item.prev = item.prev;
		}
		else
		{
			auto const prev_idx(item.prev);
			m_last_1 = 1 + prev_idx;
			if (change_size)
				m_items.pop_back();
		}
	}
	
	
	template <typename t_value>
	void array_list <t_value>::push_back(t_value const &value)
	{
		item_type item(value);
		add_item(std::move(item));
	}
	
	
	template <typename t_value>
	void array_list <t_value>::push_back(t_value &&value)
	{
		item_type item(std::move(value));
		add_item(std::move(item));
	}
	
	
	template <typename t_value>
	void array_list <t_value>::link(t_value const &value, size_type const idx, size_type const prev, size_type const next)
	{
		item_type item(value);
		item.prev = prev;
		item.next = next;
		link_item(std::move(item), idx);
	}
	
	
	template <typename t_value>
	void array_list <t_value>::link(t_value &&value, size_type const idx, size_type const prev, size_type const next)
	{
		item_type item(std::move(value));
		item.prev = prev;
		item.next = next;
		link_item(std::move(item), idx);
	}
	
	
	template <typename t_value>
	void array_list <t_value>::add_item(item_type &&item)
	{
		auto const size(m_items.size());
		if (0 == size)
			m_first = 0;
		else
		{
			auto const &prev_item(m_items.back());
			prev_item.next = size;
			item.prev = size - 1;
		}
		
		m_items.push_back(std::move(item));
		m_last_1 = m_items.size();
	}
	
	
	template <typename t_value>
	void array_list <t_value>::link_item(item_type &&item, size_type const idx)
	{
		assert(idx < m_items.size());
		
		if (SIZE_MAX != item.prev)
			m_items[item.prev].next = idx;
		
		if (SIZE_MAX != item.next)
			m_items[item.next].prev = idx;
		
		if (idx < m_first)
			m_first = idx;
		
		m_items[idx] = std::move(item);
		m_last_1 = std::max(m_last_1, 1 + idx);
	}
	
	
	template <typename t_value>
	template <typename t_archive>
	void array_list <t_value>::boost_load(t_archive &ar, unsigned int const version)
	{
		ar & m_first;
		ar & m_last_1;
		
		// pair_type has a reference type as the second element, which prevents default initialization of the type.
		// The reference isn’t serialized, though, so we can substitute it with the actual type.
		typedef typename detail::array_list_pair_iterator_detail <array_list <t_value>>::pair_type pair_type;
		typedef std::pair <
			typename pair_type::first_type,
			std::remove_reference_t <typename pair_type::second_type>
		> stored_pair_type;
		
		std::vector <stored_pair_type> buffer;
		ar & buffer;
		
		m_items.resize(1 + m_last_1);
		size_type prev_idx(SIZE_MAX);
		for (auto &pair : buffer)
		{
			link(std::move(pair.second), pair.first, prev_idx);
			prev_idx = pair.first;
		}
	}
	
	
	template <typename t_value>
	template <typename t_archive>
	void array_list <t_value>::boost_save(t_archive &ar, unsigned int const version) const
	{
		ar & m_first;
		ar & m_last_1;
		
		typedef typename detail::array_list_pair_iterator_detail <array_list <t_value> const>::pair_type pair_type;
		std::vector <pair_type> buffer;
		for (auto const &pair : this->const_pair_iterator_proxy())
			buffer.emplace_back(pair);
		
		ar & buffer;
	}
}


namespace boost { namespace serialization {
	
	template <typename t_archive, typename t_value>
	void load(t_archive &ar, libbio::array_list <t_value> &array_list, unsigned int const version)
	{
		array_list.boost_load(ar, version);
	}
	
	
	template <typename t_archive, typename t_value>
	void save(t_archive &ar, libbio::array_list <t_value> const &array_list, unsigned int const version)
	{
		array_list.boost_save(ar, version);
	}
	
	
	template <typename t_archive, typename t_value>
	void serialize(t_archive &ar, libbio::array_list <t_value> &array_list, unsigned int version)
	{
		// See <boost/serialization/split_free.hpp>; the definition of BOOST_SERIALIZATION_SPLIT_FREE
		// is essentially the same as this function.
		split_free(ar, array_list, version);
	}
}}

#endif

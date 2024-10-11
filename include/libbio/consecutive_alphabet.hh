/*
 * Copyright (c) 2018-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_CONSECUTIVE_ALPHABET_HH
#define LIBBIO_CONSECUTIVE_ALPHABET_HH

#include <array>
#include <boost/sort/sort.hpp>
#include <cstdint>
#include <iostream>
#include <libbio/assert.hh>
#include <libbio/dispatch.hh>
#include <libbio/utility.hh>
#include <map>
#include <vector>


namespace libbio {
	template <typename t_char, template <typename> typename t_map>
	class consecutive_alphabet_mt;
}

// Forward declaration needed for template friends.
namespace boost::serialization {
	template <typename t_archive, typename t_char, template <typename> typename t_map>
	void serialize(t_archive &, libbio::consecutive_alphabet_mt <t_char, t_map> &, unsigned int const);
}


namespace libbio::detail {

	// Map characters to consecutive integers using arrays.
	struct consecutive_alphabet_as_helper
	{
		// Use a helper class to add a static assertion.
		template <typename t_char>
		struct map_type_helper
		{
			static_assert(std::is_unsigned_v <t_char>);
			
			using map_type = std::array <
				t_char,
				1 + std::numeric_limits <t_char>::max()
			>;
		};
		
		// Declare the map type.
		template <typename t_char>
		using map_type = typename map_type_helper <t_char>::map_type;
	};
	
	
	// Map characters to consecutive integers using std::map.
	struct consecutive_alphabet_map_helper
	{
		template <typename t_char>
		using map_type = std::map <t_char, t_char>;
	};
	
	
	template <bool t_parallel>
	struct consecutive_alphabet_as_parallel_builder_helper
	{
		template <typename t_iterator>
		void sort(t_iterator begin, t_iterator end);
		
		template <typename t_builder, typename t_forward_iterable>
		void prepare(t_builder &builder, t_forward_iterable const &text);
	};
	
	
	template <>
	struct consecutive_alphabet_as_parallel_builder_helper <false>
	{
		template <typename t_iterator>
		void sort(t_iterator begin, t_iterator end);
		
		template <typename t_builder, typename t_forward_iterable>
		void prepare(t_builder &builder, t_forward_iterable const &text);
	};
}


namespace libbio {
	
	template <typename t_alphabet>
	class consecutive_alphabet_mt_builder
	{
	protected:
		typedef typename t_alphabet::map_type map_type;
		
	protected:
		t_alphabet					m_alphabet;
		
	public:
		t_alphabet &alphabet() { return m_alphabet; }
		t_alphabet const &alphabet() const { return m_alphabet; }
		
	protected:
		map_type &to_comp() { return m_alphabet.m_to_comp; }
		map_type &to_char() { return m_alphabet.m_to_char; }
	};
	
	
	// Map characters to consecutive integers using the given map type.
	template <typename t_char, template <typename> typename t_map>
	class consecutive_alphabet_mt
	{
		template <typename>
		friend class consecutive_alphabet_mt_builder;
		
		template <typename t_archive, typename t_tpl_char, template <typename> typename t_tpl_map>
		friend void boost::serialization::serialize(t_archive &ar, consecutive_alphabet_mt <t_tpl_char, t_tpl_map> &alphabet, unsigned int const version);
		
	public:
		enum { ALPHABET_MAX = std::numeric_limits <t_char>::max() };
		
	protected:
		typedef t_map <t_char>	map_type;
		
	protected:
		map_type				m_to_comp{};
		map_type				m_to_char{};
		
	public:
		consecutive_alphabet_mt() = default;
		inline std::size_t sigma() const { return m_to_char.size(); }	// Nul character is always considered.
		inline t_char char_to_comp(t_char const c) const { return m_to_comp[c]; }
		inline t_char comp_to_char(t_char const c) const { return m_to_char[c]; }
		
		// For debugging.
		void print();
		
		void swap(consecutive_alphabet_mt &other);
		
	protected:
		template <typename t_archive>
		void boost_serialize(t_archive &ar, unsigned int const version);
	};
	
	
	// Map with std::arrays.
	template <typename t_char, typename t_builder = void>
	using consecutive_alphabet_as = consecutive_alphabet_mt <t_char, detail::consecutive_alphabet_as_helper::map_type>;
	
	// Map with std::maps.
	template <typename t_char, typename t_builder = void>
	using consecutive_alphabet_map = consecutive_alphabet_mt <t_char, detail::consecutive_alphabet_map_helper::map_type>;
	
	
	// Create a compressed alphabet.
	template <typename t_char>
	class consecutive_alphabet_as_builder final : public consecutive_alphabet_mt_builder <consecutive_alphabet_as <t_char>>
	{
	protected:
		typedef consecutive_alphabet_as <t_char>	alphabet_type;
		
	protected:
		std::vector <bool>				m_seen;
		std::vector <t_char>			m_found_characters;
		
	public:
		consecutive_alphabet_as_builder() = default;
		
		void init();
		
		template <typename t_forward_iterable>
		void prepare(t_forward_iterable const &text);
		
		void compress();
	};
	
	
	// Create a compressed alphabet, parallelize where possible.
	template <typename t_char>
	class consecutive_alphabet_as_parallel_builder final : public consecutive_alphabet_mt_builder <consecutive_alphabet_as <t_char>>
	{
		template <bool>
		friend struct detail::consecutive_alphabet_as_parallel_builder_helper;
		
	protected:
		typedef consecutive_alphabet_as <t_char>								alphabet_type;
		typedef std::atomic <smallest_unsigned_lockfree_type_gte_t <t_char>>	counter_type;
		
	protected:
		std::vector <std::atomic_flag>	m_flags;
		std::vector <t_char>			m_found_characters;
		counter_type					m_found_char_idx{1}; // Consider the NUL character.
		
	public:
		consecutive_alphabet_as_parallel_builder() = default;
		
		void init();
		
		// Map the characters in the given text to consecutive integers.
		// May be called multiple times; the text may be passed in multiple buffers.
		template <typename t_forward_iterable, bool t_parallel = true>
		void prepare(t_forward_iterable const &text);
		
		template <bool t_parallel = true>
		void compress();
		
		// Non-template variant.
		void compress();
	};
}


namespace libbio {
	
	template <typename t_char, template <typename> typename t_map>
	void swap(
		consecutive_alphabet_mt <t_char, t_map> &lhs,
		consecutive_alphabet_mt <t_char, t_map> &rhs
	)
	{
		lhs.swap(rhs);
	}
	
	
	template <typename t_char, template <typename> typename t_map>
	void consecutive_alphabet_mt <t_char, t_map>::swap(consecutive_alphabet_mt &other)
	{
		using std::swap;
		swap(m_to_comp, other.m_to_comp);
		swap(m_to_char, other.m_to_char);
	}
	
	
	template <typename t_char, template <typename> typename t_map>
	void consecutive_alphabet_mt <t_char, t_map>::print()
	{
		for (std::size_t i(0), count(m_to_char.size()); i < count; ++i)
		{
			t_char const c(m_to_char[i]);
			std::cerr << "[" << i << "]: '" << c << "' ("
			<< std::hex << +c << std::dec << ")\n";

#ifndef LIBBIO_NDEBUG
			auto const comp(m_to_comp[c]);
			libbio_assert(comp == i);
#endif
		}
	}
	
	
	template <typename t_char, template <typename> typename t_map>
	template <typename t_archive>
	void consecutive_alphabet_mt <t_char, t_map>::boost_serialize(t_archive &ar, unsigned int const version)
	{
		ar & m_to_comp;
		ar & m_to_char;
	}
	
	
	template <typename t_char>
	void consecutive_alphabet_as_builder <t_char>::init()
	{
		m_seen.resize(1 + alphabet_type::ALPHABET_MAX, 0);
		m_seen[0] = 1;
		m_found_characters.push_back('\0');
	}
	
	
	template <typename t_char>
	void consecutive_alphabet_as_parallel_builder <t_char>::init()
	{
		m_found_characters.resize(1 + alphabet_type::ALPHABET_MAX, '\0');
		
		std::vector <std::atomic_flag> temp_flags(1 + alphabet_type::ALPHABET_MAX);
		for (auto &flag : temp_flags)
			flag.clear();
		temp_flags[0].test_and_set();
		
		m_flags = std::move(temp_flags);
	}
	
	
	template <typename t_char>
	template <typename t_forward_iterable>
	void consecutive_alphabet_as_builder <t_char>::prepare(t_forward_iterable const &text)
	{
		for (auto const c : text)
		{
			if (false == m_seen[c])
			{
				// Mark the character as listed.
				m_seen[c] = true;
				m_found_characters.push_back(c);
			}
		}
	}
	
	
	// Map the characters in the given text to consecutive integers.
	// May be called multiple times; the text may be passed in multiple buffers.
	template <typename t_char>
	template <typename t_forward_iterable, bool t_parallel>
	void consecutive_alphabet_as_parallel_builder <t_char>::prepare(t_forward_iterable const &text)
	{
		detail::consecutive_alphabet_as_parallel_builder_helper <t_parallel> helper;
		helper.prepare(*this, text);
	}
	
	
	template <typename t_char>
	void consecutive_alphabet_as_builder <t_char>::compress()
	{
		// Use O(σ log σ) time to sort the characters.
		std::sort(m_found_characters.begin(), m_found_characters.end());
		
		auto &to_comp(this->to_comp());
		auto &to_char(this->to_char());
		
		std::size_t char_idx(0);
		for (auto const c : m_found_characters)
		{
			libbio_assert(1 == m_seen[c]);
			
			to_comp[c] = char_idx;
			to_char[char_idx] = c;
			++char_idx;
		}
	}
	
	
	template <typename t_char>
	template <bool t_parallel>
	void consecutive_alphabet_as_parallel_builder <t_char>::compress()
	{
		// Use O(σ log σ) time to sort the characters.
		// Parallel likely not needed unless t_char is uint16_t.
		std::size_t const count(m_found_char_idx);
		std::size_t char_idx(0);
		
		detail::consecutive_alphabet_as_parallel_builder_helper <t_parallel> helper;
		helper.sort(m_found_characters.begin(), count + m_found_characters.begin());
		
		auto &to_comp(this->to_comp());
		auto &to_char(this->to_char());
		
		for (std::size_t i(0); i < count; ++i)
		{
			auto const c(m_found_characters[i]);
			libbio_assert(m_flags[c].test_and_set());
			to_comp[c] = char_idx;
			to_char[char_idx] = c;
			++char_idx;
		}
	}
	
	
	template <typename t_char>
	void consecutive_alphabet_as_parallel_builder <t_char>::compress()
	{
		std::size_t const count(m_found_char_idx);
		if (count < 64) // FIXME: better threshold?
			compress <false>();
		else
			compress <true>();
	}
}


namespace libbio::detail {
	
	template <bool t_parallel>
	template <typename t_iterator>
	void consecutive_alphabet_as_parallel_builder_helper <t_parallel>::sort(t_iterator begin, t_iterator end)
	{
#if LIBBIO_USE_STD_PARALLEL_FOR_EACH
		std::sort(std::execution::par, begin, end);
#else
		boost::sort::block_indirect_sort(begin, end);
#endif
	}
	
	
	template <typename t_iterator>
	void consecutive_alphabet_as_parallel_builder_helper <false>::sort(t_iterator begin, t_iterator end)
	{
		std::sort(begin, end);
	}
	
	
	template <bool t_parallel>
	template <typename t_builder, typename t_forward_iterable>
	void consecutive_alphabet_as_parallel_builder_helper <t_parallel>::prepare(t_builder &builder, t_forward_iterable const &text)
	{
		parallel_for_each(
			text,
			[this, &builder](auto const c) {
				if (0 == builder.m_flags[c].test_and_set())
				{
					// Mark the character as listed.
					builder.m_found_characters[builder.m_found_char_idx++] = c;
				}
			}
		);
	}
	
	
	template <typename t_builder, typename t_forward_iterable>
	void consecutive_alphabet_as_parallel_builder_helper <false>::prepare(t_builder &builder, t_forward_iterable const &text)
	{
		for (auto const c : text)
		{
			if (0 == builder.m_flags[c].test_and_set())
			{
				// Mark the character as listed.
				builder.m_found_characters[builder.m_found_char_idx++] = c;
			}
		}
	}
}


namespace boost::serialization {
	
	template <typename t_archive, typename t_char, template <typename> typename t_map>
	void serialize(t_archive &ar, libbio::consecutive_alphabet_mt <t_char, t_map> &alphabet, unsigned int const version)
	{
		alphabet.boost_serialize(ar, version);
	}
}

#endif

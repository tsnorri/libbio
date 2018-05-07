/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_CONSECUTIVE_ALPHABET_HH
#define LIBBIO_CONSECUTIVE_ALPHABET_HH

#include <array>
#include <cstdint>
#include <iostream>
#include <map>


namespace libbio { namespace detail {

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
		
		// Provide custom initialization to the alphabet.
		struct delegate
		{
			// Zero-fill the arrays.
			template <typename t_alphabet>
			void init(t_alphabet &alphabet)
			{
				alphabet.m_to_comp.fill(0);
				alphabet.m_to_char.fill(0);
			}
		};
	};
	
	// Map characters to consecutive integers using std::map.
	struct consecutive_alphabet_map_helper
	{
		template <typename t_char>
		using map_type = std::map <t_char, t_char>;
	};
}}


namespace libbio {
	
	// Map characters to consecutive integers using the given map type.
	template <typename t_char, template <typename> typename t_map, typename t_delegate = void>
	class consecutive_alphabet_mt
	{
		friend t_delegate;
		
	protected:
		t_map <t_char>			m_to_comp;
		t_map <t_char>			m_to_char;
		std::vector <t_char>	m_found_characters;
		std::size_t				m_char_idx{1};
		
	public:
		template <typename = std::enable_if_t <!std::is_void_v <t_delegate>>>
		consecutive_alphabet_mt()
		{
			t_delegate delegate;
			delegate.init(*this);
		}
		
		inline std::size_t sigma() const { return m_char_idx; }	// Nul character is always considered.
		inline t_char char_to_comp(t_char const c) const { return m_to_comp[c]; }
		inline t_char comp_to_char(t_char const c) const { return m_to_char[c]; }
		
		// Map the characters in the given text to consecutive integers.
		// May be called multiple times; the text may be passed in multiple buffers.
		template <typename t_forward_iterable>
		void prepare(t_forward_iterable const &text)
		{
			for (auto const c : text)
			{
				if (0 == m_to_comp[c])
				{
					// Mark the character as listed.
					m_to_comp[c] = 1;
					m_found_characters.push_back(c);
				}
			}
		}
		
		void compress()
		{
			// Use O(σ log σ) time to sort the characters.
			std::sort(m_found_characters.begin(), m_found_characters.end());
			
			for (auto const c : m_found_characters)
			{
				assert(1 == m_to_comp[c]);
				
				auto const idx(m_char_idx);
				m_to_comp[c] = idx;
				m_to_char[idx] = c;
				++m_char_idx;
			}
		}
		
		// For debugging.
		void print()
		{
			for (std::size_t i(0); i < m_char_idx; ++i)
			{
				t_char const c(m_to_char[i]);
				std::cerr << "[" << i << "]: '" << c << "' ("
				<< std::hex << +c << std::dec << ")\n";

#ifndef NDEBUG
				auto const comp(m_to_comp[c]);
				assert(comp == i);
#endif
			}
		}
	};
	
	
	template <typename t_char>
	using consecutive_alphabet_as = consecutive_alphabet_mt <
		t_char,
		detail::consecutive_alphabet_as_helper::map_type,
		detail::consecutive_alphabet_as_helper::delegate
	>;
	
	template <typename t_char>
	using consecutive_alphabet_map = consecutive_alphabet_mt <
		t_char,
		detail::consecutive_alphabet_map_helper::map_type
	>;
}

#endif

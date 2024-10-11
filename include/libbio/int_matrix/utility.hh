/*
 * Copyright (c) 2018–2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_INT_MATRIX_UTILITY_HH
#define LIBBIO_INT_MATRIX_UTILITY_HH

#include <libbio/assert.hh>
#include <libbio/int_matrix/slice.hh>
#include <libbio/utility.hh>


namespace libbio::matrices {
	
	template <typename t_matrix>
	void copy_to_word_aligned(
		::libbio::detail::int_matrix_slice <t_matrix> const &src,
		::libbio::detail::int_matrix_slice <t_matrix> &dst
	)
	{
		libbio_assert(src.size() <= dst.size());
		libbio_always_assert(dst.is_word_aligned_at_start());
		auto dst_it(dst.word_begin());
		src.to_word_range().apply_aligned([&dst_it](auto word, std::size_t element_count){
			dst_it->store(word);
			++dst_it;
		});
	}
	
	
	template <typename t_matrix>
	void copy_to_word_aligned(
		::libbio::detail::int_matrix_slice <t_matrix> const &src,
		::libbio::detail::int_matrix_slice <t_matrix> &&dst	// Proxy, allow moving.
	)
	{
		copy_to_word_aligned(src, dst);
	}
	
	
	template <typename t_src_matrix, typename t_dst_matrix>
	void transpose_column_to_row(
		::libbio::detail::int_matrix_slice <t_src_matrix> const &src,
		::libbio::detail::int_matrix_slice <t_dst_matrix> &dst
	)
	{
		libbio_assert(src.size() <= dst.size());
		auto dst_it(dst.begin());
		
		auto const &src_range(src.to_word_range());
		src_range.apply_aligned(
			[
#ifndef LIBBIO_NDEBUG
			 	&dst,
#endif
				&dst_it,
				&src
			](typename t_src_matrix::word_type word, std::size_t const element_count){ // FIXME: memory_order_acquire?
				auto const element_mask(src.matrix().element_mask());
				auto const element_bits(src.matrix().element_bits());
				for (std::size_t i(0); i < element_count; ++i)
				{
					libbio_assert(dst_it != dst.end());
					dst_it->fetch_or(word & element_mask); // FIXME: memory_order_release?
					word >>= element_bits;
					++dst_it;
				}
			}
		);
	}
	
	
	template <typename t_src_matrix, typename t_dst_matrix>
	void transpose_column_to_row(
		::libbio::detail::int_matrix_slice <t_src_matrix> const &src,
		::libbio::detail::int_matrix_slice <t_dst_matrix> &&dst	// Proxy, allow moving.
	)
	{
		transpose_column_to_row(src, dst);
	}

	
	template <unsigned int t_pattern_length, typename t_matrix>
	void fill_column_with_bit_pattern(
		::libbio::detail::int_matrix_slice <t_matrix> &column,
		typename t_matrix::word_type pattern
	)
	{
		pattern = ::libbio::fill_bit_pattern <t_pattern_length>(pattern);
		auto word_range(column.to_word_range());
		word_range.apply_parts(
			[pattern](auto &atomic){ libbio_do_and_assert_eq(atomic.fetch_or(pattern), 0); },
			[pattern](auto &atomic, std::size_t const offset, std::size_t const length){
				libbio_assert(length);
				auto p(pattern);
				p >>= (t_matrix::WORD_BITS - length);
				p <<= offset;
				libbio_do_and_assert_eq(atomic.fetch_or(p), 0);
			}
		);
	}
	
	
	template <unsigned int t_pattern_length, typename t_matrix>
	void fill_column_with_bit_pattern(
		::libbio::detail::int_matrix_slice <t_matrix> &&column,	// Proxy, allow moving.
		typename t_matrix::word_type pattern
	)
	{
		fill_column_with_bit_pattern <t_pattern_length>(column, pattern);
	}
}

#endif

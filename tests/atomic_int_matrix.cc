/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <catch2/catch.hpp>
#include <cstdint>
#include <libbio/int_matrix.hh>
#include <range/v3/view/zip.hpp>

namespace gen	= Catch::Generators;
namespace lb	= libbio;


namespace {
	
	template <typename t_val>
	constexpr std::uint16_t u16(t_val val) { return val; }


	template <std::uint8_t t_bits, typename t_word>
	struct matrix_helper
	{
		lb::atomic_int_matrix <t_bits, t_word>	matrix;

		matrix_helper(std::size_t const rows, std::size_t const columns):
			matrix(rows, columns)
		{
			std::uint8_t val{};
			for (auto ref : matrix)
				ref.fetch_or((val++) & 0xf);
		}

		static matrix_helper create_8() { return matrix_helper(4, 2); }
		static matrix_helper create_7x3() { return matrix_helper(7, 3); }
	};

	typedef matrix_helper <4, std::uint16_t> matrix_helper_t;
}


SCENARIO("A atomic_int_matrix may be created", "[atomic_int_matrix]")
{
	GIVEN("an atomic_int_matrix <4, std::uint16_t>")
	{
		auto hh(matrix_helper_t::create_8());
		auto &matrix(hh.matrix);
		
		WHEN("queried for size")
		{
			THEN("it returns the correct values")
			{
				REQUIRE(16 == matrix.word_bits());
				REQUIRE(4 == matrix.element_bits());
				REQUIRE(4 == matrix.element_count_in_word());
				REQUIRE(8 == matrix.size());
				REQUIRE(2 == matrix.word_size());
			}
		}
	}
}


SCENARIO("Values may be stored into an atomic_int_matrix", "[atomic_int_matrix]")
{
	GIVEN("an atomic_int_matrix <4, std::uint16_t>")
	{
		auto hh(matrix_helper_t::create_8());
		auto &matrix(hh.matrix);
		
		REQUIRE(matrix.size() == 8);
		REQUIRE(matrix.number_of_rows() == 4);
		REQUIRE(matrix.number_of_columns() == 2);
		
		WHEN("queried")
		{
			THEN("the matrix returns the correct values with operator()")
			{
				std::uint8_t val{};
				for (std::size_t i(0), columns(matrix.number_of_columns()); i < columns; ++i)
				{
					for (std::size_t j(0), rows(matrix.number_of_rows()); j < rows; ++j)
						REQUIRE(matrix(j, i) == ((val++) & 0xf));
				}
			}
			
			THEN("the matrix returns the correct values with a range-based for loop")
			{
				std::uint8_t val{};
				for (auto const k : matrix)
					REQUIRE(((val++) & 0xf) == k);
			}
		}
	}
}


SCENARIO("Packed matrix slices return the correct values", "[atomic_int_matrix]")
{
	GIVEN("an atomic_int_matrix <4, std::uint16_t>")
	{
		auto hh(matrix_helper_t::create_8());
		auto &matrix(hh.matrix);
		
		REQUIRE(matrix.size() == 8);
		REQUIRE(matrix.number_of_rows() == 4);
		REQUIRE(matrix.number_of_columns() == 2);
		
		WHEN("accessed by columns")
		{
			auto const &tup = GENERATE(gen::table <std::size_t, std::array <std::uint16_t, 4>>({
				{0U, lb::make_array <std::uint16_t>(u16(0), u16(1), u16(2), u16(3))},
				{1U, lb::make_array <std::uint16_t>(u16(4), u16(5), u16(6), u16(7))}
			}));
			
			auto const idx(std::get <0>(tup));
			auto const &expected(std::get <1>(tup));
			
			auto const &column(matrix.column(idx));
			REQUIRE(expected.size() == column.size());
			
			THEN("the correct values will be returned when accessed with a range-based for loop")
			{
				for (auto const &pair : ranges::view::zip(column, expected))
					REQUIRE(pair.first == pair.second);
			}
			
			THEN("the correct value will be returned when accessed with operator[]")
			{
				for (std::size_t i(0); i < expected.size(); ++i)
					REQUIRE(expected[i] == column[i]);
			}
		}
		
		WHEN("accessed by rows")
		{
			auto const &tup = GENERATE(gen::table <std::size_t, std::array <std::uint16_t, 2>>({
				{0U, lb::make_array <std::uint16_t>(u16(0), u16(4))},
				{1U, lb::make_array <std::uint16_t>(u16(1), u16(5))},
				{2U, lb::make_array <std::uint16_t>(u16(2), u16(6))},
				{3U, lb::make_array <std::uint16_t>(u16(3), u16(7))}
			}));
			
			auto const idx(std::get <0>(tup));
			auto const &expected(std::get <1>(tup));
			
			auto const &row(matrix.row(idx));
			REQUIRE(expected.size() == row.size());
			
			THEN("the correct values will be returned when accessed with a range-based for loop")
			{
				for (auto const &pair : ranges::view::zip(row, expected))
					REQUIRE(pair.first == pair.second);
			}
			
			THEN("the correct value will be returned when accessed with operator[]")
			{
				for (std::size_t i(0); i < row.size(); ++i)
					REQUIRE(expected[i] == row[i]);
			}
		}
		
		WHEN("accessed by column with start and limit")
		{
			THEN("the correct values will be returned")
			{
				auto column(matrix.column(1, 1, 3));
				REQUIRE(2 == column.size());
				REQUIRE(5 == column[0]);
				REQUIRE(6 == column[1]);
			}
		}
		
		WHEN("accessed by row with start and limit")
		{
			THEN("the correct values will be returned")
			{
				auto row(matrix.row(2, 1, 2));
				REQUIRE(1 == row.size());
				REQUIRE(6 == row[0]);
			}
		}
	}
}


SCENARIO("Unaligned packed matrix slices return the correct values", "[atomic_int_matrix]")
{
	GIVEN("an atomic_int_matrix <4, std::uint16_t>")
	{
		auto hh(matrix_helper_t::create_7x3());
		auto &matrix(hh.matrix);
		
		REQUIRE(matrix.size() == 21);
		REQUIRE(matrix.number_of_rows() == 7);
		REQUIRE(matrix.number_of_columns() == 3);
		
		WHEN("accessed by column")
		{
			THEN("the correct values will be returned")
			{
				auto const &column(matrix.column(1));
				REQUIRE(7 == column.size());
				REQUIRE(7 == column[0]);
				REQUIRE(8 == column[1]);
				REQUIRE(11 == column[4]);
				REQUIRE(12 == column[5]);
			}
		}
		
		WHEN("accessed by row")
		{
			THEN("the correct values will be returned")
			{
				auto const &row(matrix.row(3));
				REQUIRE(3 == row.size());
				REQUIRE(3 == row[0]);
				REQUIRE(10 == row[1]);
				REQUIRE(1 == row[2]);
			}
		}
		
		WHEN("accessed by column with start and limit")
		{
			THEN("the correct values will be returned")
			{
				auto const &column(matrix.column(1, 3, 6));
				REQUIRE(3 == column.size());
				REQUIRE(10 == column[0]);
				REQUIRE(11 == column[1]);
				REQUIRE(12 == column[2]);
			}
		}
		
		WHEN("accessed by row with start and limit")
		{
			THEN("the correct values will be returned")
			{
				auto const &row(matrix.row(5, 1, 3));
				REQUIRE(2 == row.size());
				REQUIRE(12 == row[0]);
				REQUIRE(3 == row[1]);
			}
		}
	}
}


SCENARIO("Packed matrix columns may be filled using matrices::fill_Column_with_bit_pattern", "[atomic_int_matrix]")
{
	GIVEN("an atomic_int_matrix <2, std::uint8_t> with two columns")
	{
		lb::atomic_int_matrix <2, std::uint8_t> mat(5, 2);
		for (std::size_t i(0); i < mat.number_of_columns(); ++i)
		{
			for (auto const val : mat.column(i))
				REQUIRE(0 == val);
		}
		
		WHEN("a column is filled")
		{
			auto const &idxs = GENERATE(gen::values <std::pair <std::size_t, std::size_t>>({
				{0, 1},
				{1, 0}
			}));
			
			auto const dst_idx(idxs.first);
			auto const other_idx(idxs.second);
			lb::matrices::fill_column_with_bit_pattern <2>(mat.column(dst_idx), 0x1);
			THEN("the columns will have the correct values")
			{
				for (auto const val : mat.column(dst_idx))
					REQUIRE(0x1 == val);
				for (auto const val : mat.column(other_idx))
					REQUIRE(0x0 == val);
			}
		}
	}
	
	GIVEN("an atomic_int_matrix <2, std::uint8_t> with three columns")
	{
		lb::atomic_int_matrix <2, std::uint8_t> mat(5, 3);
		for (std::size_t i(0); i < mat.number_of_columns(); ++i)
		{
			for (auto const val : mat.column(i))
				REQUIRE(0 == val);
		}
		
		auto const fill_value = GENERATE(gen::values <std::uint16_t>({0x0, 0x1, 0x2, 0x3}));
		WHEN("a column is filled")
		{
			auto const &idxs = GENERATE(gen::table <std::size_t, std::array <std::size_t, 2>>({
				{0, lb::make_array <std::size_t>(1U, 2U)},
				{1, lb::make_array <std::size_t>(0U, 2U)},
				{2, lb::make_array <std::size_t>(0U, 1U)}
			}));
			
			auto const dst_idx(std::get <0>(idxs));
			auto const &other_idxs(std::get <1>(idxs));
			lb::matrices::fill_column_with_bit_pattern <2>(mat.column(dst_idx), fill_value);
			THEN("the columns will have the correct values")
			{
				for (auto const val : mat.column(dst_idx))
					REQUIRE(fill_value == val);
				
				for (auto const idx : other_idxs)
				{
					for (auto const val : mat.column(idx))
						REQUIRE(0x0 == val);
				}
			}
		}
	}
}


SCENARIO("Packed matrix columns may be transposed using matrices::transpose_column_to_row()", "[atomic_int_matrix]")
{
	GIVEN("two packed_matrices <4, std::uint16_t>")
	{
		auto hh(matrix_helper_t::create_7x3());
		lb::atomic_int_matrix <4, std::uint16_t> dst(2, 8);
		auto &src(hh.matrix);
		
		auto const col_idx = GENERATE(gen::values <std::size_t>({0, 1, 2}));
		auto const &src_col(src.column(col_idx));
		
		WHEN("a source column is transposed")
		{
			auto const row_idx = GENERATE(gen::values <std::size_t>({0, 1}));
			auto dst_row(dst.row(row_idx));
			
			lb::matrices::transpose_column_to_row(src_col, dst_row);
			
			THEN("the destination row will have the correct values")
			{
				std::size_t i(0);
				while (i < src_col.size())
				{
					REQUIRE(src_col[i] == dst_row[i]);
					++i;
				}
				while (i < dst_row.size())
				{
					REQUIRE(0 == dst_row[i]);
					++i;
				}
			}
		}
	}
	
	
	GIVEN("atomic_int_matrix <2, std::uint16_t> as a source and atomic_int_matrix <4, std::uint16_t> as a destination")
	{
		// Source smaller than word size but word-aligned.
		lb::atomic_int_matrix <2, std::uint16_t> src(4, 4);
		lb::atomic_int_matrix <4, std::uint16_t> dst(4, 4);
		
		auto const col_idx = GENERATE(gen::values <std::size_t>({0, 1, 2, 3}));
		src(0, col_idx).fetch_or(0x3);
		src(1, col_idx).fetch_or(0x0);
		src(2, col_idx).fetch_or(0x1);
		src(3, col_idx).fetch_or(0x2);
		
		WHEN("a source column is transposed")
		{
			auto const &idxs = GENERATE(gen::table <std::size_t, std::array <std::size_t, 3>>({
				{0U, lb::make_array <std::size_t>(1U, 2U, 3U)},
				{1U, lb::make_array <std::size_t>(0U, 2U, 3U)},
				{2U, lb::make_array <std::size_t>(0U, 1U, 3U)},
				{3U, lb::make_array <std::size_t>(0U, 1U, 2U)},
			}));
			
			auto const row_idx(std::get <0>(idxs));
			auto const &other_idxs(std::get <1>(idxs));
			lb::matrices::transpose_column_to_row(src.column(col_idx), dst.row(row_idx));
			
			THEN("the destionation row will have the correct values")
			{
				for (auto const idx : other_idxs)
				{
					auto const &row(dst.row(idx));
					for (auto const val : row)
						REQUIRE(0 == val);
				}
				
				REQUIRE(0x3 == dst(row_idx, 0));
				REQUIRE(0x0 == dst(row_idx, 1));
				REQUIRE(0x1 == dst(row_idx, 2));
				REQUIRE(0x2 == dst(row_idx, 3));
			}
		}
	}
}


SCENARIO("Packed matrix contents may be copied with matrices::copy_to_word_aligned", "[atomic_int_matrix]")
{
	GIVEN("two packed_matrices <2, std::uint32_t>")
	{
		lb::atomic_int_matrix <2, std::uint32_t> src(16, 1);
		lb::atomic_int_matrix <2, std::uint32_t> dst(16, 1);
	
		WHEN("values are copied using copy_to_word_aligned()")
		{
			src(1, 0).fetch_or(0x1);
			src(2, 0).fetch_or(0x2);
			src(3, 0).fetch_or(0x3);
			
			auto const col(src.column(0, 1, 4));
			lb::matrices::copy_to_word_aligned(col, dst.column(0));
			
			THEN("the destination will have the correct values")
			{
				REQUIRE(0x1 == dst(0, 0));
				REQUIRE(0x2 == dst(1, 0));
				REQUIRE(0x3 == dst(2, 0));
				for (std::size_t i(3); i < dst.number_of_rows(); ++i)
					REQUIRE(0x0 == dst(i, 0));
			}
		}
		
		WHEN("values are copied using copy_to_word_aligned() (with the source containing extra values)")
		{
			src(1, 0).fetch_or(0x1);
			src(2, 0).fetch_or(0x2);
			src(3, 0).fetch_or(0x3);
			// Extra values that should not be copied.
			src(4, 0).fetch_or(0x1);
			src(5, 0).fetch_or(0x1);
			src(6, 0).fetch_or(0x1);
			
			auto const col(src.column(0, 1, 4));
			lb::matrices::copy_to_word_aligned(col, dst.column(0));
			
			THEN("the destination will have the correct values")
			{
				REQUIRE(0x1 == dst(0, 0));
				REQUIRE(0x2 == dst(1, 0));
				REQUIRE(0x3 == dst(2, 0));
				for (std::size_t i(3); i < dst.number_of_rows(); ++i)
					REQUIRE(0x0 == dst(i, 0));
			}
		}
	}
}

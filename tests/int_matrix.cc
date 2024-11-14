/*
 * Copyright (c) 2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <cstddef>
#include <libbio/int_matrix.hh>

namespace lb	= libbio;


namespace {

	template <typename t_matrix>
	void fill_matrix(t_matrix &matrix)
	{
		auto it(matrix.begin());
		auto const end(matrix.end());
		std::size_t val{};
		while (it != end)
		{
			++val;
			*it |= val;
		}
	}
}


TEMPLATE_TEST_CASE(
	"int_matrix can be copied and moved",
	"[template][int_matrix]",
	(unsigned char),
	(unsigned short),
	(unsigned int),
	(unsigned long),
	(unsigned long long)
)
{
	// We use 8 bits (i.e. CHAR_BIT) per value.
	constexpr static std::size_t const matrix_size(32);
	constexpr static std::size_t const matrix_rows(matrix_size / 8);

	SECTION("int_matrix can be copied")
	{
		lb::int_matrix <8, TestType> matrix(matrix_rows, matrix_rows);
		REQUIRE(1 < matrix.word_size());
		auto const matrix_(matrix);

		CHECK(matrix.number_of_rows() == matrix_.number_of_rows());
		CHECK(matrix.number_of_columns() == matrix_.number_of_columns());
		CHECK(matrix == matrix_);
	}

	SECTION("int_matrix can be moved")
	{
		lb::int_matrix <8, TestType> matrix(matrix_rows, matrix_rows);
		REQUIRE(1 < matrix.word_size());
		auto const matrix_(matrix);
		lb::int_matrix <8, TestType> matrix__ = std::move(matrix);

		CHECK(matrix__.number_of_rows() == matrix_.number_of_rows());
		CHECK(matrix__.number_of_columns() == matrix_.number_of_columns());
		CHECK(matrix__ == matrix_);
	}
}

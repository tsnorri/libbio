/*
 * Copyright (c) 2018-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <cstddef>
#include <cstdint>
#include <libbio/matrix.hh>

namespace lb	= libbio;


namespace {
	template <typename t_value>
	void create_matrix_12(lb::matrix <t_value> &dst)
	{
		std::size_t const rows(3);
		std::size_t const columns(4);

		lb::matrix <t_value> temp(rows, columns);
		t_value val{};
		for (std::size_t i(0); i < columns; ++i)
		{
			for (std::size_t j(0); j < rows; ++j)
			{
				auto &ref(temp(j, i));
				ref = val++;
			}
		}

		using std::swap;
		swap(temp, dst);
	}
}


TEMPLATE_TEST_CASE(
	"Templated matrix tests",
	"[template][matrix]",
	std::int8_t,
	std::int16_t,
	std::int32_t,
	std::int64_t,
	std::uint8_t,
	std::uint16_t,
	std::uint32_t,
	std::uint64_t,
	float,
	double
) {

	SECTION("Matrices may be created")
	{
		GIVEN("an empty matrix")
		{
			lb::matrix <TestType> matrix;
			WHEN("queried")
			{
				THEN("the matrix returns the correct size")
				{
					REQUIRE(matrix.size() == 0);
					REQUIRE(matrix.number_of_rows() == 1);
					REQUIRE(matrix.number_of_columns() == 0);
				}
			}
		}

		GIVEN("a non-empty matrix")
		{
			lb::matrix <TestType> matrix(3, 4);
			WHEN("queried")
			{
				THEN("the matrix returns the correct size")
				{
					REQUIRE(matrix.size() == 12);
					REQUIRE(matrix.number_of_rows() == 3);
					REQUIRE(matrix.number_of_columns() == 4);
				}
			}
		}
	}

	SECTION("Matrices can store values")
	{
		GIVEN("a filled matrix")
		{
			lb::matrix <TestType> matrix;
			create_matrix_12(matrix);
			WHEN("queried")
			{
				THEN("the matrix returns the correct values")
				{
					TestType val{};
					for (std::size_t i(0), columns(matrix.number_of_columns()); i < columns; ++i)
					{
						for (std::size_t j(0), rows(matrix.number_of_rows()); j < rows; ++j)
							REQUIRE(matrix(j, i) == val++);
					}

				}
			}

			WHEN("cell is assigned a new value")
			{
				REQUIRE(matrix(1, 3) == 10);
				matrix(1, 3) = 9;
				THEN("the new value will be returned")
				{
					REQUIRE(matrix(1, 3) == 9);
				}
			}
		}
	}

	SECTION("Matrix slices may be used to access the matrix")
	{
		GIVEN("a filled matrix")
		{
			lb::matrix <TestType> matrix;
			create_matrix_12(matrix);

			WHEN("the rows are queried")
			{
				THEN("the correct values are returned")
				{
					REQUIRE(matrix.number_of_rows() == 3);
					for (std::size_t i(0), rows(matrix.number_of_rows()); i < rows; ++i)
					{
						auto const slice(matrix.row(i));
						REQUIRE(4 == std::distance(slice.begin(), slice.end()));

						auto it(slice.begin());
						auto end(slice.end());

						for (std::size_t j(0); j < 4; ++j)
						{
							auto const expected(j * 3 + i);
							REQUIRE(lb::is_equal(expected, *it++));
						}

						REQUIRE(it == end);
					}
				}
			}

			WHEN("the columns are queried")
			{
				THEN("the correct values are returned")
				{
					REQUIRE(matrix.number_of_columns() == 4);
					TestType val{};
					for (std::size_t i(0), columns(matrix.number_of_columns()); i < columns; ++i)
					{
						auto const slice(matrix.column(i));
						REQUIRE(3 == std::distance(slice.begin(), slice.end()));

						auto it(slice.begin());
						auto end(slice.end());

						for (std::size_t j(0); j < 3; ++j)
							REQUIRE(*it++ == val++);

						REQUIRE(it == end);
					}
				}
			}

			WHEN("a cell is assigned a new value using a row")
			{
				auto slice(matrix.row(1));

				REQUIRE(slice[3] == 10);
				slice[3] = 9;
				THEN("the new value will be returned when accessing")
				{
					REQUIRE(slice[3] == 9);
					REQUIRE(matrix(1, 3) == 9);
				}
			}

			WHEN("a cell is assigned a new value using a column")
			{
				auto slice(matrix.column(3));

				REQUIRE(slice[1] == 10);
				slice[1] = 9;
				THEN("the new value will be returned when accessing")
				{
					REQUIRE(slice[1] == 9);
					REQUIRE(matrix(1, 3) == 9);
				}
			}

			WHEN("a row is accessed using iterators and std::min_element")
			{
				auto const slice(matrix.row(1));
				auto it(std::min_element(slice.begin(), slice.end()));
				THEN("the minimum element will be found")
				{
					REQUIRE(*it == 1);
				}
			}

			WHEN("a column is accessed using iterators and std::min_element")
			{
				auto const slice(matrix.column(3));
				auto it(std::min_element(slice.begin(), slice.end()));
				THEN("the minimum element will be found")
				{
					REQUIRE(*it == 9);
				}
			}
		}
	}
}

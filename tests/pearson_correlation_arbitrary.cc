/*
 * Copyright (c) 2026 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <boost/math/statistics/bivariate_statistics.hpp>
#include <cmath>
#include <cstddef>
#include <ostream>
#include <libbio/algorithm.hh>
#include <libbio/assert.hh>
#include <libbio/rapidcheck_test_driver.hh>
#include <limits>
#include <vector>

namespace bms	= boost::math::statistics;
namespace lb	= libbio;


namespace {

	template <typename t_value>
	struct correlation_input
	{
		typedef t_value value_type;

		std::vector <value_type> lhs;
		std::vector <value_type> rhs;
	};


	template <typename t_value>
	std::ostream &operator<<(std::ostream &os, correlation_input <t_value> const &input)
	{
		os << "\nlhs:";
		for (auto const ll : input.lhs) os << ' ' << ll;
		os << "\nrhs:";
		for (auto const rr : input.rhs) os << ' ' << rr;
		return os;
	}


	template <typename t_value>
	bool is_equal_fp(t_value lhs, t_value rhs, t_value const multiplier = 1.0, t_value epsilon = std::numeric_limits <t_value>::epsilon())
	requires std::is_floating_point_v <t_value>
	{
		// Idea from https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/
		// FIXME: take more special cases into account?
		// FIXME: Combine with the function in sam_reader.cc.
		using std::fabs;
		auto const diff(fabs(lhs - rhs));
		auto const lhsa(fabs(lhs));
		auto const rhsa(fabs(rhs));
		auto const max(std::max(lhsa, rhsa));
		return diff <= max * multiplier * std::numeric_limits <t_value>::epsilon();
	}
}


namespace rc {

	template <typename t_value>
	struct Arbitrary <correlation_input <t_value>>
	{
		static Gen <correlation_input <t_value>> arbitrary()
		{
			return gen::withSize([](int size) {
				return gen::construct <correlation_input <t_value>>(
					gen::container <std::vector <t_value>>(size, gen::arbitrary <t_value>()),
					gen::container <std::vector <t_value>>(size, gen::arbitrary <t_value>())
				);
			});
		}
	};
}


TEMPLATE_TEST_CASE(
	"pearson_correlation_calculator can calculate correlation",
	"[template][pearson_correlation_calculator]",
	correlation_input <double>
)
{
	return lb::rc_check(
		"pearson_correlation_calculator can calculate correlation",
		[](TestType const &input){
			if (input.lhs.empty())
				return;

			lb::pearson_correlation_coefficient_calculator <typename TestType::value_type> pc{};
			pc.init(input.lhs.front(), input.rhs.front());
			for (std::size_t ii{1}; ii < input.lhs.size(); ++ii)
				pc.update(input.lhs[ii], input.rhs[ii]);

			auto const actual{pc.correlation()};
			auto const expected{bms::correlation_coefficient(input.lhs, input.rhs)};
			RC_LOG() << "Expected: " << expected << " actual: " << actual << '\n';
			RC_ASSERT((std::isnan(expected) and std::isnan(actual)) or is_equal_fp(expected, actual));
		}
	);
}


TEMPLATE_TEST_CASE(
	"pearson_correlation_calculator can calculate correlation with pairwise update",
	"[template][pearson_correlation_calculator]",
	std::vector <correlation_input <double>>
)
{
	return lb::rc_check(
		"pearson_correlation_calculator can calculate correlation",
		[](TestType const &input){
			typedef typename TestType::value_type::value_type value_type;
			typedef std::vector <value_type> value_vector;
			typedef lb::pearson_correlation_coefficient_calculator <value_type> calculator_type;

			value_vector combined_lhs;
			value_vector combined_rhs;
			for (auto const &input_pair : input)
			{
				combined_lhs.append_range(input_pair.lhs);
				combined_rhs.append_range(input_pair.rhs);
			}

			libbio_assert_eq(combined_lhs.size(), combined_rhs.size());
			if (combined_lhs.empty())
			{
				RC_TAG(0);
				return;
			}

			// Update the given calculator.
			auto const handle_input([](calculator_type &pc, correlation_input <value_type> const &input){
				libbio_assert_eq(input.lhs.size(), input.rhs.size());
				if (input.lhs.empty())
					return;

				pc.init(input.lhs.front(), input.rhs.front());
				for (std::size_t ii{1}; ii < input.lhs.size(); ++ii)
					pc.update(input.lhs[ii], input.rhs[ii]);
			});

			// Calculate the values for each partition.
			std::vector <calculator_type> pcs(input.size());
			for (std::size_t ii{}; ii < input.size(); ++ii)
				handle_input(pcs[ii], input[ii]);

			// Find the first non-empty calculator.
			auto const first_idx{[&](){
				for (std::size_t ii{}; ii < pcs.size(); ++ii)
					if (not pcs[ii].empty())
						return ii;
				libbio_fail();
			}()};

			// Pairwise update.
			auto &pc{pcs[first_idx]};
			std::size_t non_empty_count{1};
			for (std::size_t ii{first_idx + 1}; ii < pcs.size(); ++ii)
			{
				if (pcs[ii].empty())
					continue;

				pc.pairwise_update(pcs[ii]);
				++non_empty_count;
			}

			RC_TAG(non_empty_count);

			auto const actual{pc.correlation()};
			auto const expected{bms::correlation_coefficient(combined_lhs, combined_rhs)};
			RC_LOG() << "Expected: " << expected << " actual: " << actual << '\n';
			RC_ASSERT((std::isnan(expected) and std::isnan(actual)) or is_equal_fp(expected, actual, 5000.0));
		}
	);
}

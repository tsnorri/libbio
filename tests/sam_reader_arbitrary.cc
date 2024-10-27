/*
 * Copyright (c) 2023-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <cstddef>
#include <cstdint>
#include <utility>
#if !(defined(LIBBIO_NO_SAM_READER) && LIBBIO_NO_SAM_READER)

#include <array>
#include <libbio/markov_chain.hh>
#include <libbio/sam.hh>
#include <numeric>
#include <range/v3/view/enumerate.hpp>
#include <range/v3/view/generate.hpp>
#include <range/v3/view/iota.hpp>
#include <range/v3/view/zip.hpp>
#include <sstream>
#include <string>
#include <vector>
#include "rapidcheck_test_driver.hh"

namespace lb		= libbio;
namespace mcs		= libbio::markov_chains;
namespace rsv		= ranges::views;
namespace sam		= libbio::sam;
namespace tuples	= libbio::tuples;


namespace {

	std::string const reference_id_first_characters{"01234567890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!#$%&+./:;?@^_|~-"};
	std::string const reference_id_characters{"01234567890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!#$%&*+./:;=?@^_|~-"};
	std::string const sequence_characters{"ACGT"};

	constexpr std::array const sort_order_types{
		sam::sort_order_type::unknown,
		sam::sort_order_type::unsorted,
		sam::sort_order_type::queryname,
		sam::sort_order_type::coordinate
	};

	constexpr std::array const grouping_types{
		sam::grouping_type::none,
		sam::grouping_type::query,
		sam::grouping_type::reference
	};

	constexpr std::array const molecule_topology_types{
		sam::molecule_topology_type::unknown,
		sam::molecule_topology_type::linear,
		sam::molecule_topology_type::circular
	};


	struct identifier
	{
		std::string value;
	};


	// For setting up sam::header properly.
	struct header_container
	{
		sam::header	header;

		header_container(sam::header &&header_, std::vector <std::string> &&program_ids):
			header(std::move(header_))
		{
			header.reference_sequence_identifiers.resize(header.reference_sequences.size());
			std::iota(header.reference_sequence_identifiers.begin(), header.reference_sequence_identifiers.end(), 0);

			for (auto &&[entry, id] : rsv::zip(header.programs, program_ids))
				entry.id = std::move(id);

			for (std::size_t i{1}; i < header.programs.size(); ++i)
				header.programs[i].prev_id = header.programs[i - 1].id;
		}
	};


	struct empty_cigar
	{
		constexpr static inline bool const should_skip{true};
	};

	template <sam::cigar_operation t_operation, bool t_consumes_query, bool t_is_final = false>
	struct cigar
	{
		constexpr static inline auto const operation{t_operation};
		constexpr static inline bool const should_skip{false};
		constexpr static inline bool const is_final{t_is_final};

		sam::cigar_run::count_type count{};

		cigar(sam::cigar_run::count_type const count_):
			count(count_)
		{
		}

		constexpr operator sam::cigar_run() const { return {t_operation, count}; }
		constexpr sam::cigar_run::count_type query_consumed_count() const { return t_consumes_query ? count : 0; }
	};

	using sam::operator ""_cigar_operation;

	template <typename t_base = cigar <'H'_cigar_operation, false, true>>
	struct cigar_final_h_ : public t_base { using t_base::t_base; };

	typedef cigar <'M'_cigar_operation, true>	cigar_m;
	typedef cigar <'I'_cigar_operation, true>	cigar_i;
	typedef cigar <'D'_cigar_operation, false>	cigar_d;
	typedef cigar <'N'_cigar_operation, false>	cigar_n;
	typedef cigar <'S'_cigar_operation, true>	cigar_s;
	typedef cigar <'H'_cigar_operation, false>	cigar_h;
	typedef cigar <'P'_cigar_operation, false>	cigar_p;
	typedef cigar <'='_cigar_operation, true>	cigar_eq;
	typedef cigar <'N'_cigar_operation, true>	cigar_x;
	struct cigar_initial_h : public cigar_h { using cigar_h::cigar_h; };
	struct cigar_initial_s : public cigar_s { using cigar_s::cigar_s; };
	typedef cigar_final_h_ <> cigar_final_h;
	struct cigar_final_s : public cigar_s { using cigar_s::cigar_s; };


	struct empty {}; // Not actually used since we don’t instantiate the chain.

	typedef mcs::chain <
		empty,
		empty_cigar,
		mcs::join_transition_lists <
			// From initial
			mcs::transition_list <
				mcs::transition <empty_cigar, cigar_initial_h, 0.1>,
				mcs::transition <empty_cigar, cigar_initial_s, 0.1>
			>,
			mcs::transitions_to_any <empty_cigar, 0.8, cigar_m, cigar_i, cigar_d, cigar_n, cigar_p, cigar_eq, cigar_x>,
			// From initial H
			mcs::transitions_to_any <cigar_initial_h, 1.0, cigar_initial_s, cigar_m, cigar_i, cigar_d, cigar_n, cigar_p, cigar_eq, cigar_x>,
			// From initial S
			mcs::transitions_to_any <cigar_initial_s, 1.0, cigar_m, cigar_i, cigar_d, cigar_n, cigar_p, cigar_eq, cigar_x>,
			// Middle
			mcs::transitions_between_any <0.8, cigar_m, cigar_i, cigar_d, cigar_n, cigar_p, cigar_eq, cigar_x>,
			mcs::transitions_from_any <cigar_final_s, 0.1, cigar_m, cigar_i, cigar_d, cigar_n, cigar_p, cigar_eq, cigar_x>,
			mcs::transitions_from_any <cigar_final_h, 0.1, cigar_m, cigar_i, cigar_d, cigar_n, cigar_p, cigar_eq, cigar_x>,
			// From final S to final H
			mcs::transition_list <
				mcs::transition <cigar_final_s, cigar_final_h, 1.0>
			>
		>
	> cigar_chain_type;


	struct tag_id
	{
		std::uint16_t value{};
	};


	typedef std::vector <sam::record> record_vector;


	struct record_set
	{
		sam::header		header;
		record_vector	records;

		explicit record_set(header_container &&hc):
			header(std::move(hc.header))
		{
		}

		record_set(header_container &&hc, record_vector &&records_):
			header(std::move(hc.header)),
			records(std::move(records_))
		{
		}

		record_set(record_set const &other, record_vector const &records_):
			header(other.header),
			records(records_)
		{
		}
	};

	std::ostream &operator<<(std::ostream &os, record_set const &rs)
	{
		os << rs.header;
		for (auto const &rec : rs.records)
		{
			output_record(os, rs.header, rec);
			os << '\n';
		}
		return os;
	}
}

namespace rc {

	template <>
	struct Arbitrary <std::byte>
	{
		static Gen <std::byte> arbitrary()
		{
			return gen::map(gen::inRange(0, 256), [](unsigned char const cc){
				return std::byte{cc};
			});
		}
	};


	template <>
	struct Arbitrary <identifier>
	{
		static Gen <identifier> arbitrary()
		{
			return gen::construct <identifier>(
				gen::nonEmpty(
					gen::container <std::string>(
						gen::map(gen::inRange('!', char('~' - 2)), [](char const cc) -> char {
							// Skip ‘*’ and ‘@’.
							switch (cc)
							{
								case '@': return '|';
								case '*': return '}';
								default: return cc;
							}
						})
					)
				)
			);
		}
	};
}


namespace {
	auto make_identifier()
	{
		return rc::gen::map(rc::gen::arbitrary <identifier>(), [](auto &&id){ return std::move(id.value); });
	}


	auto make_reference_id(sam::reference_id_type const ref_count)
	{
		return rc::gen::weightedOneOf <sam::reference_id_type>({
			{1, rc::gen::just(sam::INVALID_REFERENCE_ID)},
			{8, rc::gen::inRange(sam::reference_id_type(0), ref_count)}
		});
	}


	auto make_quality(std::size_t const seq_len)
	{
		return rc::gen::weightedOneOf <std::vector <char>>({
			{1, rc::gen::just(std::vector <char>{})}, // QUAL unavailable, i.e. “*”
			{8, rc::gen::map(
				rc::gen::container <std::vector <char>>(seq_len, rc::gen::inRange('!', '~')),
				[](auto &&vec){
					// The SAM format has a deficiency in the edge case where the sequence length is one
					// and the quality score is 9, which is encoded as “*”.
					if (1 == vec.size() && '*' == vec.front())
						vec.front() = ')';
					return std::move(vec);
				}
			)}
		});
	}
}


namespace rc {

	template <>
	struct Arbitrary <sam::sort_order_type>
	{
		static Gen <sam::sort_order_type> arbitrary()
		{
			return gen::elementOf(sort_order_types);
		}
	};


	template <>
	struct Arbitrary <sam::grouping_type>
	{
		static Gen <sam::grouping_type> arbitrary()
		{
			return gen::elementOf(grouping_types);
		}
	};



	template <>
	struct Arbitrary <sam::molecule_topology_type>
	{
		static Gen <sam::molecule_topology_type> arbitrary()
		{
			return gen::elementOf(molecule_topology_types);
		}
	};


	template <>
	struct Arbitrary <sam::reference_sequence_entry>
	{
		static Gen <sam::reference_sequence_entry> arbitrary()
		{
			return gen::apply(
				[](auto &&entry, char first_identifier_character){
					libbio_assert(!entry.name.empty());
					entry.name[0] = first_identifier_character;
					return std::move(entry);
				},
				gen::construct <sam::reference_sequence_entry>(
					gen::nonEmpty(gen::container <std::string>(gen::elementOf(reference_id_characters))),
					gen::arbitrary <sam::position_type>(),
					gen::arbitrary <sam::molecule_topology_type>()
				),
				gen::elementOf(reference_id_first_characters)
			);
		}
	};


	template <>
	struct Arbitrary <sam::read_group_entry>
	{
		static Gen <sam::read_group_entry> arbitrary()
		{
			return gen::construct <sam::read_group_entry>(
				make_identifier(),
				gen::container <std::string>(gen::inRange(' ', '~'))
			);
		}
	};


	template <>
	struct Arbitrary <sam::program_entry>
	{
		static Gen <sam::program_entry> arbitrary()
		{
			return gen::construct <sam::program_entry>(
				gen::just(std::string{}),
				gen::container <std::string>(gen::inRange(' ', '~')),
				gen::container <std::string>(gen::inRange(' ', '~')),
				gen::just(std::string{}),
				gen::container <std::string>(gen::inRange(' ', '~')),
				gen::container <std::string>(gen::inRange(' ', '~'))
			);
		}
	};


	template <>
	struct Arbitrary <sam::header>
	{
		static Gen <sam::header> arbitrary()
		{
			// Currently we only test parsing and ignore the values of sort order and grouping.
			return gen::construct <sam::header>(
				gen::uniqueBy <sam::reference_sequence_entry_vector>(
					gen::arbitrary <sam::reference_sequence_entry>(),
					[](sam::reference_sequence_entry const &entry){
						return entry.name;
					}
				),
				gen::arbitrary <sam::read_group_entry_vector>(),
				gen::arbitrary <sam::program_entry_vector>(),
				gen::container <std::vector <std::string>>(
					gen::container <std::string>(gen::inRange(' ', '~'))
				),
				gen::just(sam::header::reference_sequence_identifier_vector{}),	// Filled later.
				gen::arbitrary <std::uint16_t>(),								// version_major
				gen::arbitrary <std::uint16_t>(),								// version_minor
				gen::arbitrary <sam::sort_order_type>(),
				gen::arbitrary <sam::grouping_type>()
			);
		}
	};


	template <>
	struct Arbitrary <header_container>
	{
		static Gen <header_container> arbitrary()
		{
			return gen::mapcat(gen::arbitrary <sam::header>(), [](auto &&header){
				return gen::construct <header_container>(
					gen::just(std::move(header)),
					gen::unique <std::vector <std::string>>(header.programs.size(), make_identifier()) // Program identifiers
				);
			});
		}
	};


	template <>
	struct Arbitrary <tag_id>
	{
		static Gen <tag_id> arbitrary()
		{
			char const ac('Z' - 'A' + 1);
			char const nc('9' - '0' + 1);
			return gen::apply(
				[](char const v1, char const v2) -> tag_id {
					std::uint16_t retval{};

					if (v1 < ac) retval = v1 + 'A';
					else retval = v1 + 'a' - ac;
					libbio_assert(('A' <= retval && retval <= 'Z') || ('a' <= retval && retval <= 'z'));

					retval <<= 8;

					if (v2 < nc) retval |= v2 + '0';
					else if (v2 < nc + ac) retval |= v2 + 'A' - nc;
					else retval |= v2 + 'a' - ac - nc;
					libbio_assert([c1 = retval & 0xff](){
						return(('A' <= c1 && c1 <= 'Z') || ('a' <= c1 && c1 <= 'z') || ('0' <= c1 && c1 <= '9'));
					}());

					return {retval};
				},
				gen::inRange(0, 2 * ac),
				gen::inRange(0, 2 * ac + nc)
			);
		}
	};


	template <typename t_type, bool t_should_clear_elements>
	struct Arbitrary <sam::detail::vector_container <t_type, t_should_clear_elements>>
	{
		static Gen <sam::detail::vector_container <t_type, t_should_clear_elements>> arbitrary()
		{
			typedef sam::detail::vector_container <t_type, t_should_clear_elements> return_type;
			return gen::map(gen::arbitrary <std::vector <t_type>>(), [](auto &&vec){
				auto const size(vec.size());
				return return_type{std::move(vec), size};
			});
		}
	};


	template <>
	struct Arbitrary <sam::optional_field>
	{
		static Gen <sam::optional_field> arbitrary()
		{
			return gen::mapcat(gen::arbitrary <sam::optional_field::value_tuple_type>(), [](auto &&value_tuple){

				// Fix std::uint32_t values so that they can be output as SAM.
				for (auto &val : std::get <sam::optional_field::container_of_t <std::uint32_t>>(value_tuple))
				{
					if (INT32_MAX < val)
						val = INT32_MAX;
				}

				// Count the generated values.
				std::size_t value_count{};
				tuples::visit_parameters <sam::optional_field::value_tuple_type>([&value_count, &value_tuple]<typename t_type>(){
					value_count += std::get <t_type>(value_tuple).size();
				});

				// Generate the tag ranks.
				return gen::apply(
					[value_count](auto &&value_tuple, auto const &tag_ids) -> sam::optional_field {
						// Make sure that we have only valid characters.
						auto const transform_1([](unsigned char const cc){ return cc % (127 - '!') + '!'; });
						auto const transform_2([](unsigned char const cc){ return cc % (127 - ' ') + ' '; });

						for (auto &cc : std::get <sam::optional_field::container_of_t <char>>(value_tuple))
							cc = transform_1(cc);

						for (auto &str : std::get <sam::optional_field::container_of_t <std::string>>(value_tuple).values)
						{
							for (auto &cc : str)
								cc = transform_2(cc);
						}

						// Ranks.
						sam::optional_field::tag_rank_vector tag_ranks;
						tag_ranks.reserve(value_count);

						// Fill the ranks.
						std::size_t tag_idx{};
						std::size_t type_idx{};
						tuples::visit_parameters <sam::optional_field::value_tuple_type>([&value_tuple, &tag_ids, &tag_ranks, &tag_idx, &type_idx]<typename t_type>(){
							for (auto const rank : rsv::iota(0U, std::get <t_type>(value_tuple).size()))
							{
								tag_ranks.emplace_back(tag_ids[tag_idx].value, type_idx, rank);
								++tag_idx;
							}
							++type_idx;
						});

						return sam::optional_field(std::move(tag_ranks), std::move(value_tuple));
					},
					gen::just(std::move(value_tuple)),
					gen::uniqueBy <std::vector <tag_id>>(
						value_count,
						gen::arbitrary <tag_id>(),
						[](auto const tag_id){ return tag_id.value; }
					)
				);
			});
		};
	};


	template <>
	struct Arbitrary <sam::record>
	{
		static Gen <sam::record> arbitrary()
		{
			return gen::mapcat(gen::container <std::vector <char>>(gen::elementOf(sequence_characters)), [](auto &&seq){
				auto const seq_len(seq.size());
				return gen::build <sam::record>(
					gen::construct <sam::record>(),
					gen::set(
						&sam::record::qname,
						gen::weightedOneOf <std::string>({
							{1, gen::just(std::string{})}, // Replaced with *
							{8, make_identifier()}
						})
					),
					gen::set(
						&sam::record::cigar,
						gen::exec([seq_len](){
							auto remaining(seq_len);
							std::vector <sam::cigar_run> retval;
							if (!remaining)
								return retval;

							cigar_chain_type::visit_node_types(
								rsv::generate([] -> double { return double(*gen::inRange <std::uint32_t>(0, UINT32_MAX)) / UINT32_MAX; }), // gen::inRange <double> does not seem to work.
								[&retval, &remaining]<typename t_type> -> bool {
									if constexpr (t_type::should_skip)
										return true;
									else if constexpr (t_type::is_final)
									{
										retval.emplace_back(t_type(sam::cigar_run::count_type(remaining)));
										return false;
									}
									else
									{
										sam::cigar_run::count_type const count(1 + *gen::inRange(
											sam::cigar_run::count_type(0),
											sam::cigar_run::count_type(remaining)
										));
										t_type run(count);
										retval.emplace_back(run);
										remaining -= run.query_consumed_count();
										return 0 != remaining;
									}
								}
							);
							return retval;
						})
					),
					gen::set(&sam::record::seq, gen::just(std::move(seq))),
					gen::set(&sam::record::qual, make_quality(seq_len)),
					gen::set(&sam::record::optional_fields, gen::arbitrary <sam::optional_field>()),
					gen::set(&sam::record::pos, gen::inRange <sam::position_type>(0, INT32_MAX)),
					gen::set(&sam::record::pnext, gen::inRange <sam::position_type>(0, INT32_MAX)),
					gen::set(&sam::record::tlen, gen::arbitrary <std::int32_t>()),
					gen::set(&sam::record::flag, gen::arbitrary <std::uint16_t>()),
					gen::set(&sam::record::mapq, gen::arbitrary <sam::mapping_quality_type>())
				);
			});
		}
	};


	template <>
	struct Arbitrary <record_set>
	{
		static Gen <record_set> arbitrary()
		{
			return gen::mapcat(gen::arbitrary <header_container>(), [](header_container &&hc){
				auto const ref_count(hc.header.reference_sequences.size());
				if (0 == ref_count)
					return gen::construct <record_set>(gen::just(std::move(hc)));

				return gen::construct <record_set>(
					gen::just(std::move(hc)),
					gen::container <record_vector>(
						gen::build(
							gen::arbitrary <sam::record>(),
							gen::set(&sam::record::rname_id, make_reference_id(ref_count)),
							gen::set(&sam::record::rnext_id, make_reference_id(ref_count))
						)
					)
				);
			});
		}
	};
}


TEST_CASE(
	"sam::reader works with arbitrary input",
	"[sam_reader]"
)
{
	std::size_t iteration{};
	return lb::rc_check(
		"sam::reader works with arbitrary input",
		[&iteration](record_set const &input){
			RC_TAG(input.header.reference_sequences.size());

			++iteration;
			RC_LOG() << "Iteration: " << iteration << '\n';

			// Output the header and the records.
			std::stringstream stream;
			stream << input;

			// Parse.
			{
				sam::header parsed_header;
				std::vector <sam::record> parsed_records;
				auto const input_view(stream.view());

				RC_LOG() << "Expected (" << input_view.size() << " characters):\n" << input << '\n';

				sam::character_range input_range(stream.view());
				sam::reader reader;
				reader.read_header(parsed_header, input_range);
				reader.read_records(parsed_header, input_range, [&parsed_records](auto const &rec){
					parsed_records.emplace_back(rec);
				});

				RC_LOG() << "Actual:\n" << record_set(input, parsed_records) << '\n';

				// TODO: Compare the headers?
				RC_ASSERT(input.records.size() == parsed_records.size());
				std::vector <std::size_t> non_matching;
				for (auto const &[idx, tup] : rsv::zip(input.records, parsed_records) | rsv::enumerate)
				{
					auto const &[input_rec, parsed_rec] = tup;
					if (!sam::is_equal_(input.header, parsed_header, input_rec, parsed_rec))
						non_matching.emplace_back(idx);
				}

				if (!non_matching.empty())
				{
					RC_LOG() << "Non-matching record indices: ";
					bool is_first{true};
					for (auto const idx : non_matching)
					{
						if (!is_first)
							RC_LOG() << ", ";
						RC_LOG() << idx;
					}
					RC_LOG() << '\n';

					for (auto const idx : non_matching)
					{
						RC_LOG() << '(' << idx << ") expected v. actual (QNAMEs: “" << input.records[idx].qname << "”, “" << parsed_records[idx].qname << "”):\n";
						sam::output_record(RC_LOG(), input.header, input.records[idx]);
						RC_LOG() << '\n';
						sam::output_record(RC_LOG(), input.header, parsed_records[idx]);
						RC_LOG() << '\n';
					}

					RC_FAIL("Got non-matching records.");
				}
			}

			return true;
		}
	);
}

#endif

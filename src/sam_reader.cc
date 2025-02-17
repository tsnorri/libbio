/*
 * Copyright (c) 2023-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#if !(defined(LIBBIO_NO_SAM_READER) && LIBBIO_NO_SAM_READER)

#include <algorithm>					// std::sort
#include <array>
#include <boost/io/ios_state.hpp>		// boost::io::ios_all_saver
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iterator>
#include <libbio/assert.hh>
#include <libbio/sam/cigar.hh>
#include <libbio/sam/header.hh>
#include <libbio/sam/optional_field.hh>
#include <libbio/sam/reader.hh>
#include <libbio/sam/record.hh>
#include <libbio/utility.hh>
#include <limits>
#include <numeric>
#include <optional>
#include <ostream>
#include <range/v3/view/enumerate.hpp>
#include <range/v3/view/subrange.hpp>
#include <range/v3/view/transform.hpp>
#include <range/v3/view/zip.hpp>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <vector>
#include <version>

namespace io	= boost::io;
namespace lb	= libbio;
namespace sam	= libbio::sam;
namespace rsv	= ranges::views;


#if defined(__cpp_lib_format) && 201907L <= __cpp_lib_format
#	include <format>

namespace {
	inline auto format_optional_field_byte_array_value(unsigned char const bb)
	{
		return std::format("{:02X}", bb);
	}
}
#else
#	include <boost/format.hpp>

namespace {
	inline auto format_optional_field_byte_array_value(unsigned char const bb)
	{
		static boost::format const fmt("%02X");
		// For some reason Boost Format does not seem to honour the field width unless the value is converted to integer.
		return boost::format(fmt) % (+bb);
	}
}
#endif


namespace {
	enum
	{
		QNAME = 0,
		FLAG,
		RNAME,
		POS,
		MAPQ,
		CIGAR,
		RNEXT,
		PNEXT,
		TLEN,
		SEQ,
		QUAL,
		OPTIONAL
	};


	constexpr static std::array optional_field_type_codes_for_output{'A', 'i', 'i', 'i', 'i', 'i', 'i', 'f', 'Z', 'H', 'B', 'B', 'B', 'B', 'B', 'B', 'B'};


	template <std::size_t t_n>
	constexpr bool is_in(char const cc, std::array <char, t_n> const &arr)
	{
		return arr.end() != std::find(arr.begin(), arr.end(), cc);
	}


	inline bool check_reference_ids(
		sam::reference_id_type const lhs,
		sam::reference_id_type const rhs,
		sam::reference_sequence_entry_vector const &lhsr,
		sam::reference_sequence_entry_vector const &rhsr
	)
	{
		if (sam::INVALID_REFERENCE_ID == lhs || sam::INVALID_REFERENCE_ID == rhs)
			return lhs == rhs;

		libbio_assert_lt(lhs, lhsr.size());
		libbio_assert_lt(rhs, rhsr.size());
		return lhsr[lhs] == rhsr[rhs];
	}

	template <typename t_type>
	inline bool cmp(t_type const lhs, t_type const rhs, double const)
	requires (!std::is_floating_point_v <t_type>)
	{
		return lhs == rhs;
	}

	template <typename t_type>
	inline bool cmp(t_type const lhs, t_type const rhs, double const multiplier)
	requires std::is_floating_point_v <t_type>
	{
		// Idea from https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/
		// FIXME: take more special cases into account?
		auto const diff(std::fabs(lhs - rhs));
		auto const lhsa(std::fabs(lhs));
		auto const rhsa(std::fabs(rhs));
		auto const max(std::max(lhsa, rhsa));
		return diff <= max * multiplier * std::numeric_limits <t_type>::epsilon();
	}


	void output_record_(std::ostream &os, sam::header const &header_, sam::record const &rec)
	{
		// QNAME
		if (rec.qname.empty())
			os << '*';
		else
			os	<< rec.qname;
		os << '\t';

		// FLAG
		os << rec.flag << '\t';

		// RNAME
		if (sam::INVALID_REFERENCE_ID == rec.rname_id)
			os << '*';
		else
			os << header_.reference_sequences[rec.rname_id].name;
		os << '\t';

		// POS, MAPQ
		os	<< (1 + rec.pos) << '\t'
			<< +rec.mapq << '\t';

		// CIGAR
		if (rec.cigar.empty())
			os << '*';
		else
			std::copy(rec.cigar.begin(), rec.cigar.end(), std::ostream_iterator <sam::cigar_run>(os));
		os << '\t';

		// RNEXT
		if (sam::INVALID_REFERENCE_ID == rec.rnext_id)
			os << '*';
		else if (rec.rname_id == rec.rnext_id)
			os << '=';
		else
			os << header_.reference_sequences[rec.rnext_id].name;
		os << '\t';

		// PNEXT, TLEN
		os	<< (1 + rec.pnext) << '\t'
			<< rec.tlen << '\t';

		// SEQ
		if (rec.seq.empty())
			os << '*';
		else
			std::copy(rec.seq.begin(), rec.seq.end(), std::ostream_iterator <char>(os));
		os << '\t';

		// QUAL
		if (rec.qual.empty())
			os << '*';
		else
			std::copy(rec.qual.begin(), rec.qual.end(), std::ostream_iterator <char>(os));
	}


	template <typename t_range>
	void output_optional_field(std::ostream &os, sam::optional_field const &of, t_range &&range)
	{
		io::ios_all_saver guard(os);
		os << std::setprecision(std::numeric_limits <sam::optional_field::floating_point_type>::digits10 + 1);

		auto visitor(lb::aggregate{
			[&os]<std::size_t t_idx, char t_type_code>(auto const &val) requires (is_in(t_type_code, std::array{'c', 'C', 's', 'S', 'i', 'I'})) { os << +val; }, // Not array.
			[&os]<std::size_t t_idx, char t_type_code>(auto const &val) requires ('A' == t_type_code) { os << char(val); },
			[&os]<std::size_t t_idx, char t_type_code>(auto const &val) requires (is_in(t_type_code, std::array{'f', 'Z'})) { os << val; },
			[&os]<std::size_t t_idx, char t_type_code>(auto const &vec) requires ('H' == t_type_code) {
				for (auto const val : vec)
					os << format_optional_field_byte_array_value(static_cast <unsigned char>(val));
			},
			[&os]<std::size_t t_idx, char t_type_code, typename t_type>(t_type const &vec) requires ('B' == t_type_code) {
				typedef typename t_type::value_type element_type;
				os << sam::optional_field::array_type_code_v <element_type>;
				for (auto const val : vec)
					os << ',' << +val;
			}
		});

		bool is_first{true};
		for (auto const &tr : range)
		{
			if (is_first)
				is_first = false;
			else
				os << '\t';

			// Output integral types as signed 32-bit integer (SAMv1 § 4.2.4).
			os << char(tr.tag_id >> 8) << char(tr.tag_id & 0xff) << ':' << optional_field_type_codes_for_output[tr.type_index] << ':';
			of.visit <void>(tr, visitor);
		}
	}


	// Debugging helper.
	template <std::size_t t_idx = 0, typename t_tuple>
	std::size_t compare_tuples(t_tuple const &lhs, t_tuple const &rhs)
	{
		if constexpr (std::tuple_size_v <t_tuple> == t_idx)
			return SIZE_MAX;
		else
		{
			auto const &lhs_(std::get <t_idx>(lhs));
			auto const &rhs_(std::get <t_idx>(rhs));
			if (lhs_ != rhs_)
				return t_idx;
			return compare_tuples <1 + t_idx>(lhs, rhs);
		}
	}


	bool is_equal__(sam::header const &lhsh, sam::header const &rhsh, sam::record const &lhsr, sam::record const &rhsr)
	{
		// Make a tuple out of the directly comparable values.
		auto const directly_comparable_to_tuple([](sam::record const &rec){
			return std::tie(rec.qname, rec.cigar, rec.seq, rec.qual, rec.pos, rec.pnext, rec.tlen, rec.flag, rec.mapq);
		});

		if (SIZE_MAX != compare_tuples(directly_comparable_to_tuple(lhsr), directly_comparable_to_tuple(rhsr)))
			return false;

		if (!check_reference_ids(lhsr.rname_id, rhsr.rname_id, lhsh.reference_sequences, rhsh.reference_sequences))
			return false;

		if (!check_reference_ids(lhsr.rnext_id, rhsr.rnext_id, lhsh.reference_sequences, rhsh.reference_sequences))
			return false;

		return true;
	}
}


namespace libbio::sam::detail {

	void prepare_record(header const &header_, parser_type::record_type &src, record &dst)
	{
		using std::swap;

		swap(std::get <QNAME>(src), dst.qname);
		swap(std::get <CIGAR>(src), dst.cigar);
		swap(std::get <SEQ>(src), dst.seq);
		swap(std::get <QUAL>(src), dst.qual);
		swap(std::get <OPTIONAL>(src), dst.optional_fields);

		if ("*" == dst.qname) dst.qname.clear();
		if ("*" == std::string_view(dst.seq.begin(), dst.seq.end())) dst.seq.clear();
		if ("*" == std::string_view(dst.qual.begin(), dst.qual.end())) dst.qual.clear();

		dst.rname_id = header_.find_reference(std::get <RNAME>(src));
		if ("=" == std::get <RNEXT>(src))
			dst.rnext_id = dst.rname_id;
		else
			dst.rnext_id = header_.find_reference(std::get <RNEXT>(src));

		dst.pos = std::get <POS>(src);
		dst.pnext = std::get <PNEXT>(src);
		dst.tlen = std::get <TLEN>(src);
		dst.flag = std::get <FLAG>(src);
		dst.mapq = std::get <MAPQ>(src);

		--dst.pos; // In input 1-based, 0 for invalid.
		--dst.pnext;
	}


	void prepare_parser_record(record &src, parser_type::record_type &dst)
	{
		using std::swap;

		swap(std::get <QNAME>(dst), src.qname);
		swap(std::get <CIGAR>(dst), src.cigar);
		swap(std::get <SEQ>(dst), src.seq);
		swap(std::get <QUAL>(dst), src.qual);
		swap(std::get <OPTIONAL>(dst), src.optional_fields);
	}
}


namespace libbio::sam {

	char const *to_chars(sort_order_type const so)
	{
		switch (so)
		{
			case sort_order_type::unknown:
				return "unknown";
			case sort_order_type::unsorted:
				return "unsorted";
			case sort_order_type::queryname:
				return "queryname";
			case sort_order_type::coordinate:
				return "coordinate";
		}

		return "unknown";
	}


	char const *to_chars(grouping_type const go)
	{
		switch (go)
		{
			case grouping_type::none:
				return "none";
			case grouping_type::query:
				return "query";
			case grouping_type::reference:
				return "reference";
		}

		return "none";
	}


	std::ostream &operator<<(std::ostream &os, reference_sequence_entry const &rs)
	{
		os << "@SQ\tSN:" << rs.name << "\tLN:" << rs.length;
		switch (rs.molecule_topology)
		{
			case molecule_topology_type::unknown:
				break;
			case molecule_topology_type::linear:
				os << "\tTP:linear";
				break;
			case molecule_topology_type::circular:
				os << "\tTP:circular";
				break;
		}
		return os;
	}


	std::ostream &operator<<(std::ostream &os, read_group_entry const &rg)
	{
		os << "@RG\tID:" << rg.id;
		if (!rg.description.empty())
			os << "\tDS:" << rg.description;
		return os;
	}


	std::ostream &operator<<(std::ostream &os, program_entry const &pg)
	{
		os << "@PG\tID:" << pg.id;
		if (!pg.name.empty()) os << "\tPN:" << pg.name;
		if (!pg.command_line.empty()) os << "\tCL:" << pg.command_line;
		if (!pg.prev_id.empty()) os << "\tPP:" << pg.prev_id;
		if (!pg.description.empty()) os << "\tDS:" << pg.description;
		if (!pg.version.empty()) os << "\tVN:" << pg.version;
		return os;
	}


	std::ostream &operator<<(std::ostream &os, header const &hh)
	{
		os	<< "@HD"
			<< "\tVN:" << hh.version_major << '.' << hh.version_minor
			<< "\tSO:" << hh.sort_order
			<< "\tGO:" << hh.grouping << '\n';

		for (auto const &ref_seq : hh.reference_sequences)
			os << ref_seq << '\n';

		for (auto const &read_group : hh.read_groups)
			os << read_group << '\n';

		for (auto const &program : hh.programs)
			os << program << '\n';

		for (auto const &comment : hh.comments)
			os << "@CO\t" << comment << '\n';

		return os;
	}


	void output_record(std::ostream &os, header const &header_, record const &rec)
	{
		output_record_(os, header_, rec);

		// Optional fields
		if (!rec.optional_fields.empty())
			os << '\t' << rec.optional_fields;
	}


	void output_record_in_parsed_order(std::ostream &os, header const &header_, record const &rec, std::vector <std::size_t> &buffer)
	{
		output_record_(os, header_, rec);

		// Optional fields
		if (!rec.optional_fields.empty())
		{
			os << '\t';
			output_optional_field_in_parsed_order(os, rec.optional_fields, buffer);
		}
	}


	std::ostream &operator<<(std::ostream &os, optional_field const &of)
	{
		output_optional_field(os, of, of.m_tag_ranks);
		return os;
	}


	std::ostream &operator<<(std::ostream &os, optional_field::tag_rank const tr)
	{
		os << "tag_id: " << tr.tag_id << " type_index: " << tr.type_index << " rank: " << tr.rank << " parsed_rank: " << tr.parsed_rank;
		return os;
	}


	void output_optional_field_in_parsed_order(std::ostream &os, optional_field const &of, std::vector <std::size_t> &buffer)
	{
		buffer.resize(of.m_tag_ranks.size());
		std::iota(buffer.begin(), buffer.end(), std::size_t(0));
		std::sort(buffer.begin(), buffer.end(), [&of](auto const lhs, auto const rhs){
			return of.m_tag_ranks[lhs].parsed_rank < of.m_tag_ranks[rhs].parsed_rank;
		});
		output_optional_field(os, of, buffer | rsv::transform([&of](auto const idx){ return of.m_tag_ranks[idx]; }));
	}


	void header::assign_reference_sequence_identifiers()
	{
		reference_sequence_identifiers.clear();
		reference_sequence_identifiers.resize(reference_sequences.size());
		std::iota(reference_sequence_identifiers.begin(), reference_sequence_identifiers.end(), reference_sequence_identifier_type{});
		std::sort(reference_sequence_identifiers.begin(), reference_sequence_identifiers.end(), reference_sequence_identifier_cmp{reference_sequences});
	}


	bool is_equal(header const &lhsh, header const &rhsh, record const &lhsr, record const &rhsr)
	{
		if (!is_equal__(lhsh, rhsh, lhsr, rhsr))
			return false;
		return lhsr.optional_fields == rhsr.optional_fields;
	}


	bool is_equal_(header const &lhsh, header const &rhsh, record const &lhsr, record const &rhsr)
	{
		if (!is_equal__(lhsh, rhsh, lhsr, rhsr))
			return false;
		return lhsr.optional_fields.compare_without_type_check(rhsr.optional_fields);
	}


	bool optional_field::compare_values_strict(optional_field const &other, tag_rank const &lhsr, tag_rank const &rhsr) const
	{
		auto do_cmp([this, &other, &rhsr]<std::size_t t_idx, char t_type_code, typename t_type>(t_type const &lhs_val){
			auto const &rhs_values(std::get <t_idx>(other.m_values));
			if (! (rhsr.rank < rhs_values.size()))
				throw std::invalid_argument("Invalid rank");
			auto const &rhs_val(rhs_values[rhsr.rank]); // Get a value of the same type.
			constexpr double const epsilon_multiplier{10.0}; // FIXME: better value?
			return aggregate {
				[]<typename t_type_>(
					std::vector <t_type_> const &lhs_val,
					std::vector <t_type_> const &rhs_val
				){
					if (lhs_val.size() != rhs_val.size()) return false;
					for (auto const [lhs, rhs] : rsv::zip(lhs_val, rhs_val))
					{
						if (!cmp(lhs, rhs, epsilon_multiplier))
							return false;
					}
					return true;
				},
				[](auto const lhs, auto const rhs){
					return cmp(lhs, rhs, epsilon_multiplier);
				}
			}(lhs_val, rhs_val);
		});

		return visit <bool>(lhsr, do_cmp);
	}


	bool optional_field::compare_without_type_check(optional_field const &other) const
	{
		if (m_tag_ranks.size() != other.m_tag_ranks.size())
			return false;

		typedef std::optional <std::int32_t> common_type;
		auto const to_common_type(lb::aggregate{
			[]<std::size_t t_idx, char t_type_code>(std::uint32_t const val) -> common_type requires ('I' == t_type_code){
				if (val <= std::uint32_t(INT32_MAX))
					return {std::int32_t(val)};
				return common_type{};
			},
			[]<std::size_t t_idx, char t_type_code>(auto const &val) -> common_type requires (is_in(t_type_code, std::array{'c', 'C', 's', 'S', 'i'})) { return {std::int32_t(val)}; },
			[]<std::size_t t_idx, char t_type_code>(auto const &val) -> common_type requires (!is_in(t_type_code, std::array{'c', 'C', 's', 'S', 'i', 'I'})) { return common_type{}; }
		});

		for (auto const &[lhsr, rhsr] : rsv::zip(m_tag_ranks, other.m_tag_ranks))
		{
			if (lhsr.tag_id != rhsr.tag_id)
				return false;

			if (lhsr.type_index == rhsr.type_index)
			{
				if (!compare_values_strict(other, lhsr, rhsr))
					return false;
			}
			else
			{
				if (! (lhsr.type_index < type_codes.size() && rhsr.type_index < type_codes.size()))
					throw std::invalid_argument("Invalid type code");

				if ('i' == optional_field_type_codes_for_output[lhsr.type_index] && 'i' == optional_field_type_codes_for_output[rhsr.type_index])
				{
					auto const lhs(visit <common_type>(lhsr, to_common_type));
					auto const rhs(other.visit <common_type>(rhsr, to_common_type));
					if (! (lhs && rhs))
						return false;

					if (*lhs == *rhs)
						continue;
				}

				return false;
			}
		}

		return true;
	}


	bool optional_field::operator==(optional_field const &other) const
	{
		if (m_tag_ranks.size() != other.m_tag_ranks.size())
			return false;

		for (auto const &[lhsr, rhsr] : rsv::zip(m_tag_ranks, other.m_tag_ranks))
		{
			if (lhsr.tag_id != rhsr.tag_id) return false;
			if (lhsr.type_index != rhsr.type_index) return false;
			if (!compare_values_strict(other, lhsr, rhsr)) return false;
		}

		return true;
	}


	void optional_field::erase_values_in_range(tag_rank_vector::const_iterator rank_it, tag_rank_vector::const_iterator const rank_end)
	{
		// Precondition: the items in [rank_it, rank_end) refer to the same type.
		visit_type <void>(rank_it->type_index, aggregate {
			[rank_it, rank_end]<typename t_type>(std::vector <t_type> &vec){
				auto const end(vec.end());
				auto const it(libbio::remove_at_indices(vec.begin(), end, rank_it, rank_end, [](tag_rank const &tr){ return tr.rank; }));
				vec.erase(it, end);
			},
			[rank_it, rank_end]<typename t_type>(detail::vector_container <t_type> &vc){
				auto const begin(vc.begin());
				auto const end(vc.end());
				auto const mid(libbio::stable_partition_left_at_indices(begin, end, rank_it, rank_end, [](tag_rank const &tr){ return tr.rank; }));
				auto const new_size(std::distance(begin, mid));
				for (auto &val : ranges::subrange(mid, end))
					val.clear();

				vc.size_ = new_size;
			}
		});
	}
}

#endif

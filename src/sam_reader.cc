/*
 * Copyright (c) 2023 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#if !(defined(LIBBIO_NO_SAM_READER) && LIBBIO_NO_SAM_READER)

#include <algorithm>					// std::sort
#include <boost/io/ios_state.hpp>		// boost::io::ios_all_saver
#include <libbio/sam/reader.hh>
#include <ostream>
#include <range/v3/view/enumerate.hpp>
#include <range/v3/view/zip.hpp>
#include <version>

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
		static boost::format const fmt("%02hhX");
		return boost::format(fmt) % bb;
	}
}
#endif

namespace io	= boost::io;
namespace lb	= libbio;
namespace sam	= libbio::sam;
namespace rsv	= ranges::views;


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
		// QNAME
		if (rec.qname.empty())
			os << '*';
		else
			os	<< rec.qname;
		os << '\t';
		
		// FLAG
		os << rec.flag << '\t';
		
		// RNAME
		if (INVALID_REFERENCE_ID == rec.rname_id)
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
			std::copy(rec.cigar.begin(), rec.cigar.end(), std::ostream_iterator <cigar_run>(os));
		os << '\t';
		
		// RNEXT
		if (INVALID_REFERENCE_ID == rec.rnext_id)
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
		
		// Optional fields
		if (!rec.optional_fields.empty())
			os << '\t' << rec.optional_fields;
	}
	
	
	std::ostream &operator<<(std::ostream &os, optional_field const &of)
	{
		io::ios_all_saver guard(os);
		os << std::setprecision(std::numeric_limits <optional_field::floating_point_type>::digits10 + 1);
		
		auto visitor(aggregate{
			[&os]<std::size_t t_idx, char t_type_code>(auto const &val) requires (!('H' == t_type_code || 'B' == t_type_code)) { os << val; }, // Not array.
			[&os]<std::size_t t_idx, char t_type_code>(auto const &vec) requires ('H' == t_type_code) {
				for (auto const val : vec)
					os << format_optional_field_byte_array_value(static_cast <unsigned char>(val));
			},
			[&os]<std::size_t t_idx, char t_type_code, typename t_type>(t_type const &vec) requires ('B' == t_type_code) {
				typedef typename t_type::value_type element_type;
				os << optional_field::array_type_code_v <element_type>;
				for (auto const val : vec)
					os << ',' << +val;
			}
		});
		
		bool is_first{true};
		for (auto const &tr : of.m_tag_ranks)
		{
			if (is_first)
				is_first = false;
			else
				os << '\t';
			
			os << char(tr.tag_id >> 8) << char(tr.tag_id & 0xff) << ':' << optional_field::type_codes[tr.type_index] << ':';
			of.visit <void>(tr, visitor);
		}
		
		return os;
	}
	
	
	void header::assign_reference_sequence_identifiers()
	{
		reference_sequence_identifiers.clear();
		reference_sequence_identifiers.resize(reference_sequences.size());
		std::iota(reference_sequence_identifiers.begin(), reference_sequence_identifiers.end(), reference_sequence_identifier_type{});
		std::sort(reference_sequence_identifiers.begin(), reference_sequence_identifiers.end(), reference_sequence_identifier_cmp{reference_sequences});
	}
	
	
	void reader::prepare_record(header const &header_, parser_type::record_type &src, record &dst) const
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
	
	
	void reader::prepare_parser_record(record &src, parser_type::record_type &dst) const
	{
		using std::swap;
		
		swap(std::get <QNAME>(dst), src.qname);
		swap(std::get <CIGAR>(dst), src.cigar);
		swap(std::get <SEQ>(dst), src.seq);
		swap(std::get <QUAL>(dst), src.qual);
		swap(std::get <OPTIONAL>(dst), src.optional_fields);
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
	
	
	
	bool is_equal(header const &lhsh, header const &rhsh, record const &lhsr, record const &rhsr)
	{
		auto const directly_comparable_to_tuple([](record const &rec){
			return std::tie(rec.qname, rec.cigar, rec.seq, rec.qual, rec.pos, rec.pnext, rec.tlen, rec.flag, rec.mapq);
		});
		
		if (SIZE_MAX != compare_tuples(directly_comparable_to_tuple(lhsr), directly_comparable_to_tuple(rhsr)))
			return false;
		
		if (!check_reference_ids(lhsr.rname_id, rhsr.rname_id, lhsh.reference_sequences, rhsh.reference_sequences))
			return false;
		
		if (!check_reference_ids(lhsr.rnext_id, rhsr.rnext_id, lhsh.reference_sequences, rhsh.reference_sequences))
			return false;
		
		return lhsr.optional_fields == rhsr.optional_fields;
	}
	
	
	bool optional_field::operator==(optional_field const &other) const
	{
		if (m_tag_ranks.size() != other.m_tag_ranks.size())
			return false;
		
		for (auto const &[lhsr, rhsr] : rsv::zip(m_tag_ranks, other.m_tag_ranks))
		{
			if (lhsr.tag_id != rhsr.tag_id) return false;
			if (lhsr.type_index != rhsr.type_index) return false;
			
			auto do_cmp([this, &other, &rhsr]<std::size_t t_idx, char t_type_code, typename t_type>(t_type const &lhs_val){
				auto const &rhs_val(std::get <t_idx>(other.m_values)[rhsr.rank]);
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
			
			if (!visit <bool>(lhsr, do_cmp))
				return false;
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

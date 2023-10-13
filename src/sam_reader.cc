/*
 * Copyright (c) 2023 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <algorithm>				// std::sort
#include <libbio/sam/reader.hh>
#include <range/v3/view/enumerate.hpp>
#include <range/v3/view/zip.hpp>

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
		os	<< rec.pos << '\t'
			<< rec.mapq << '\t';
		
		// CIGAR
		if (rec.cigar.empty())
			os << '*';
		else
			std::copy(rec.cigar.begin(), rec.cigar.end(), std::ostream_iterator <cigar_run>(os));
		
		// RNEXT
		if (INVALID_REFERENCE_ID == rec.rnext_id)
			os << '*';
		else if (rec.rname_id == rec.rnext_id)
			os << '=';
		else
			os << header_.reference_sequences[rec.rnext_id].name;
		
		// PNEXT, TLEN
		os	<< rec.pnext << '\t'
			<< rec.tlen << '\t';
		
		// SEQ
		if (rec.seq.empty())
			os << '*';
		else
			std::copy(rec.seq.begin(), rec.seq.end(), std::ostream_iterator <char>(os));
		
		// QUAL
		if (rec.qual.empty())
			os << '*';
		else
			std::copy(rec.qual.begin(), rec.qual.end(), std::ostream_iterator <char>(os));
		
		// Optional fields
		os << rec.optional_fields;
	}
	
	
	std::ostream &operator<<(std::ostream &os, optional_field const &of)
	{
		auto visitor(aggregate{
			[&os]<std::size_t t_idx, char t_type_code>(auto const &val) requires (!('H' == t_type_code || 'B' == t_type_code)) { os << val; }, // Not array.
			[&os]<std::size_t t_idx, char t_type_code>(auto const &vec) requires ('H' == t_type_code) {
				os << std::hex;
				for (auto const val : vec)
					os << int(val);
				os << std::dec;
			},
			[&os]<std::size_t t_idx, char t_type_code, typename t_type>(t_type const &vec) requires ('B' == t_type_code) {
				typedef typename t_type::value_type element_type;
				os << optional_field::array_type_code_v <element_type>;
				for (auto const val : vec)
					os << ',' << val;
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
			of.visit(tr, visitor);
		}
		
		return os;
	}
	
	
	void reader::sort_reference_sequence_identifiers(header &header_) const
	{
		header_.reference_sequence_identifiers.clear();
		header_.reference_sequence_identifiers.reserve(header_.reference_sequences.size());
		
		for (auto const &[idx, ref_seq] : rsv::enumerate(header_.reference_sequences))
			header_.reference_sequence_identifiers.emplace_back(&ref_seq.name, idx);
		
		std::sort(header_.reference_sequence_identifiers.begin(), header_.reference_sequence_identifiers.end(), [](auto const &lhs, auto const &rhs){
			return *lhs.first < *rhs.first;
		});
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
	
	
	bool is_equal(header const &lhsh, header const &rhsh, record const &lhsr, record const &rhsr)
	{
		auto const directly_comparable_to_tuple([](record const &rec){
			return std::tie(rec.qname, rec.cigar, rec.seq, rec.qual, rec.pos, rec.pnext, rec.tlen, rec.flag, rec.mapq);
		});
		
		if (directly_comparable_to_tuple(lhsr) != directly_comparable_to_tuple(rhsr))
			return false;
		
		if (lhsh.reference_sequences[lhsr.rname_id] != rhsh.reference_sequences[rhsr.rname_id])
			return false;
		
		if (lhsh.reference_sequences[lhsr.rnext_id] != rhsh.reference_sequences[rhsr.rnext_id])
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
			
			bool status{true};
			auto do_cmp([this, &rhsr, &status]<std::size_t t_idx, char t_type_code, typename t_type>(t_type const &lhs_val){
				auto const &rhs_val(std::get <t_idx>(m_values)[rhsr.rank]);
				status = lhs_val == rhs_val;
			});
			
			visit(lhsr, do_cmp);
			if (!status)
				return false;
		}
		
		return true;
	}
}

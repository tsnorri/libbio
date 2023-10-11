/*
 * Copyright (c) 2023 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <algorithm>				// std::sort
#include <libbio/sam/reader.hh>
#include <range/v3/view/enumerate.hpp>

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


namespace libbio { namespace sam {
	
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
		
		dst.rname_id = header_.find_reference(std::get <RNAME>(src));
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
}}

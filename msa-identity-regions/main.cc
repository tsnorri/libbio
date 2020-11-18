/*
 * Copyright (c) 2020 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <boost/iterator/function_output_iterator.hpp>
#include <iostream>
#include <libbio/consecutive_alphabet.hh>
#include <libbio/fasta_reader.hh>
#include <libbio/pbwt_context.hh>
#include <range/v3/all.hpp>
#include <vector>
#include "cmdline.h"


namespace lb	= libbio;
namespace rsv	= ranges::view;


namespace {
	typedef std::size_t											position_type;			// For columns
	typedef std::uint16_t										string_index_type;		// For rows
	typedef std::uint8_t										character_type;
	
	typedef std::vector <character_type>						sequence;
	typedef std::vector <sequence>								sequence_vector;
	typedef libbio::consecutive_alphabet_as <character_type>	alphabet_type;
	
	typedef std::vector <string_index_type>						string_index_vector;
	typedef std::vector <position_type>							position_vector;
	
	typedef lb::pbwt::dynamic_pbwt_rmq <
		string_index_vector,
		position_vector
	>															pbwt_rmq;
	
	typedef lb::pbwt::pbwt_context <
		sequence_vector,			/* sequence_vector */
		alphabet_type,				/* alphabet_type */
		pbwt_rmq,					/* ci_rmq */
		string_index_vector,		/* string_index_vector */
		position_vector,			/* character_index_vector */
		string_index_vector,		/* count_vector */
		string_index_type			/* divergence_count_type */
	>															pbwt_context;
	
	
	class fasta_reader_delegate final : public lb::fasta_reader_delegate
	{
	protected:
		sequence_vector	*m_sequences{};
		std::size_t		m_previous_length{};
		
	public:
		fasta_reader_delegate(sequence_vector &sequences):
			m_sequences(&sequences)
		{
		}
		
		virtual bool handle_comment_line(lb::fasta_reader &reader, std::string_view const &sv) override
		{
			return true;
		}
		
		virtual bool handle_identifier(lb::fasta_reader &reader, std::string_view const &sv) override
		{
			std::cerr << "Reading sequence ‘" << sv << "’…\n";
			auto &vec(m_sequences->emplace_back());
			vec.reserve(m_previous_length);
			return true;
		}
		
		virtual bool handle_sequence_line(lb::fasta_reader &reader, std::string_view const &sv) override
		{
			auto &seq(m_sequences->back());
			std::copy(sv.begin(), sv.end(), std::back_inserter(seq));
			m_previous_length = seq.size();
			return true;
		}
	};
	
	
	struct match_record
	{
		position_type		match_length{};
		string_index_type	matched_string{};
		string_index_type	matching_string{};
		
		match_record(
			string_index_type	matched_string_,
			string_index_type	matching_string_,
			position_type		match_length_
		):
			match_length(match_length_),
			matched_string(matched_string_),
			matching_string(matching_string_)
		{
		}
	};
	
	typedef std::vector <match_record>	match_record_vector;
	
	
	// Compare match records without considering match length.
	bool operator<(match_record const &lhs, match_record const &rhs)
	{
		return std::make_tuple(lhs.matched_string, lhs.matching_string) < std::make_tuple(rhs.matched_string, rhs.matching_string);
	}
	
	
	// Output a match.
	void report_match(position_type const pos, match_record const &rec)
	{
		std::cout << pos << '\t' << rec.matched_string << '\t' << rec.matching_string << '\t' << rec.match_length << '\n';
	}
	
	
	// For use with Boost’s function output iterator.
	class match_reporter
	{
	protected:
		position_type	m_pos{};
		
	public:
		match_reporter(position_type const pos):
			m_pos(pos)
		{
		}
		
		void operator()(match_record const &rec)
		{
			report_match(m_pos - 1, rec);
		}
	};
}


int main(int argc, char **argv)
{
	gengetopt_args_info args_info;
	if (0 != cmdline_parser(argc, argv, &args_info))
		std::exit(EXIT_FAILURE);
	
	std::ios_base::sync_with_stdio(false);	// Don't use C style IO after calling cmdline_parser.
	std::cin.tie(nullptr);					// We don't require any input from the user.
	
	if (args_info.length_arg <= 0)
	{
		std::cerr << "Match length must be positive\n";
		std::exit(EXIT_FAILURE);
	}
	
	bool const ignore_n(args_info.ignore_n_flag);
	
	sequence_vector sequences;
	alphabet_type alphabet;
	
	{
		// Read the input sequences.
		fasta_reader_delegate reader_delegate(sequences);
		lb::fasta_reader reader;
		lb::mmap_handle <char> handle;
		
		handle.open(args_info.input_arg);
		reader.parse(handle, reader_delegate);
	}
	
	{
		// Build the alphabet.
		lb::consecutive_alphabet_as_builder <character_type> builder;
		
		builder.init();
		for (auto const &[i, vec] : rsv::enumerate(sequences))
		{
			std::cerr << "Handling sequence " << (1 + i) << "…\n";
			builder.prepare(vec);
		}
		builder.compress();
		
		using std::swap;
		swap(alphabet, builder.alphabet());
	}
	
	position_vector prev_n_position_1(ignore_n ? sequences.size() : 0, 0);
	
	{
		// Find the matching regions.
		std::cout << "Position\tMatched string\tMatching string\tMatch length\n";
		std::size_t const expected_match_length(args_info.length_arg);
		
		match_record_vector match_records;
		match_record_vector prev_match_records;
		
		pbwt_context pbwt_ctx(sequences, alphabet);
		pbwt_ctx.prepare();
		pbwt_ctx.process <>(
			[
				&pbwt_ctx,
				&prev_n_position_1,
				&sequences,
				&match_records,
				&prev_match_records,
				expected_match_length,
				ignore_n
			](){
				auto const &string_indices(pbwt_ctx.output_permutation());
				auto const &divergence(pbwt_ctx.output_divergence());
				auto current_matched_string(string_indices[0]);
				auto const pos(pbwt_ctx.sequence_idx()); // Character position, i.e. column.
				
				match_records.clear();
				
				// Track the rightmost position of N’s if needed.
				if (ignore_n)
				{
					auto const res(ranges::equal_range(
						string_indices,
						'N',
						ranges::less(),
						[&sequences, pos](string_index_type const idx) -> character_type {
							auto const &seq(sequences[idx]);
							return seq[pos];
						}
					));
					
					for (auto const idx : res)
						prev_n_position_1[idx] = 1 + pos;
					
					// Check the length of the matching suffix and report if needed.
					// This part is almost identical to the else branch but not quite.
					for (auto const &[str_idx, d] : rsv::zip(string_indices, divergence) | rsv::tail)
					{
						auto const match_start_1(std::max(d, prev_n_position_1[str_idx]));
						auto const match_length(1 + pos - match_start_1);
						if (expected_match_length <= match_length)
							match_records.emplace_back(current_matched_string, str_idx, match_length);
						else if (1 + pos - d < expected_match_length)
						{
							// Begin tracking the current string.
							current_matched_string = str_idx;
						}
					}
				}
				else
				{
					// Check the length of the matching suffix and report if needed.
					for (auto const &[str_idx, d] : rsv::zip(string_indices, divergence) | rsv::tail)
					{
						auto const match_length(1 + pos - d);
						if (expected_match_length <= match_length)
							match_records.emplace_back(current_matched_string, str_idx, match_length);
						else
						{
							// Begin tracking the current string.
							current_matched_string = str_idx;
						}
					}
				}
				
				// Report the matches that are missing from the current list.
				std::sort(match_records.begin(), match_records.end());
				std::set_difference(
					prev_match_records.begin(),
					prev_match_records.end(),
					match_records.begin(),
					match_records.end(),
					boost::make_function_output_iterator(match_reporter(pos))
				);
				
				using std::swap;
				swap(match_records, prev_match_records);
			}
		);
		
		// Handle the last set of records.
		{
			auto const last_pos(sequences.front().size() - 1);
			for (auto const &rec : prev_match_records)
				report_match(last_pos, rec);
		}
	}
	
	return EXIT_SUCCESS;
}

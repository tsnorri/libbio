/*
 * Copyright (c) 2020 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

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
	
	
	void report_match(
		position_type const pos,
		string_index_type const current_matched_string,
		string_index_type const str_idx,
		position_type const match_length
	)
	{
		std::cout << pos << '\t' << current_matched_string << '\t' << str_idx << '\t' << match_length << '\n';
	}
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
		pbwt_context pbwt_ctx(sequences, alphabet);
		pbwt_ctx.prepare();
		pbwt_ctx.process <>([&pbwt_ctx, &prev_n_position_1, &sequences, expected_match_length, ignore_n](){
			auto const &string_indices(pbwt_ctx.output_permutation());
			auto const &divergence(pbwt_ctx.output_divergence());
			auto current_matched_string(string_indices[0]);
			auto const pos(pbwt_ctx.sequence_idx()); // Character position, i.e. column.
			
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
						report_match(pos, current_matched_string, str_idx, match_length);
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
						report_match(pos, current_matched_string, str_idx, match_length);
					else
					{
						// Begin tracking the current string.
						current_matched_string = str_idx;
					}
				}
			}
		});
	}
	
	return EXIT_SUCCESS;
}

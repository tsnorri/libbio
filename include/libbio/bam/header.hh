/*
 * Copyright (c) 2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_BAM_HEADER_HH
#define LIBBIO_BAM_HEADER_HH

#include <cstdint>
#include <string>
#include <vector>


namespace libbio::bam {
	
	struct reference_sequence
	{
		std::string		name{};
		std::uint32_t	l_ref{};
	};
	
	
	struct header
	{
		std::vector	<reference_sequence>	reference_sequences{};
		std::string							text{};
	};
}

#endif

/*
 * Copyright (c) 2023 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_SAM_CIGAR_HH
#define LIBBIO_SAM_CIGAR_HH

#include <cstdint>
#include <libbio/assert.hh>
#include <libbio/utility.hh> // to_underlying
#include <stdexcept>


namespace libbio::sam::detail {
	inline void cigar_error_handler(char const cigar_op)
	{
		throw std::runtime_error("Unexpected operation");
	}
}


namespace libbio::sam {
	
	enum class cigar_operation : std::uint8_t
	{
		alignment_match		= 0,
		insertion			= 1,
		deletion			= 2,
		skipped_region		= 3,
		soft_clipping		= 4,
		hard_clipping		= 5,
		padding				= 6,
		sequence_match		= 7,
		sequence_mismatch	= 8
	};
	
	constexpr static std::array const cigar_operation_identifiers{'M', 'I', 'D', 'N', 'S', 'H', 'P', '=', 'X'};
	
	constexpr inline char to_char(cigar_operation const op) { return cigar_operation_identifiers[std::to_underlying(op)]; }
	
	constexpr inline cigar_operation make_cigar_operation(char const op, void(*cigar_error_handler)(char) = &detail::cigar_error_handler)
	{
		switch (op)
		{
			case 'M':
				return cigar_operation::alignment_match;
			case 'I':
				return cigar_operation::insertion;
			case 'D':
				return cigar_operation::deletion;
			case 'N':
				return cigar_operation::skipped_region;
			case 'S':
				return cigar_operation::soft_clipping;
			case 'H':
				return cigar_operation::hard_clipping;
			case 'P':
				return cigar_operation::padding;
			case '=':
				return cigar_operation::sequence_match;
			case 'X':
				return cigar_operation::sequence_mismatch;
			default:
				(*cigar_error_handler)(op);
				return cigar_operation::alignment_match;
		}
	}
	
	inline std::ostream &operator<<(std::ostream &os, cigar_operation const op) { os << cigar_operation_identifiers[to_underlying(op)]; return os; }
	
	
	class cigar_run
	{
	public:
		typedef std::uint32_t	count_type;
		
	private:
		count_type				value{};
		
	public:
		cigar_run() = default;
		
		constexpr cigar_run(cigar_operation const op, count_type const count):
			value((count_type(to_underlying(op)) << 28U) | count)
		{
			libbio_always_assert_lt(count, 1U << 28U);
		}
		
		constexpr count_type count() const { return 0xfff'ffff & value; }
		constexpr cigar_operation operation() const { return static_cast <cigar_operation>(value >> 28U); }
		constexpr bool operator==(cigar_run const other) const { return value == other.value; }
		
		constexpr void assign(count_type const count) { value = (0xf000'0000 & value) | (0xfff'ffff & count); }
		constexpr void assign(cigar_operation const op) { value = (0xfff'ffff & value) | (to_underlying(op) << 28U); }
	};
	
	
	inline std::ostream &operator<<(std::ostream &os, cigar_run const &run) { os << run.count() << run.operation(); return os; }
}

#endif

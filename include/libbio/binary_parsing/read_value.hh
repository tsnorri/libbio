/*
 * Copyright (c) 2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_BINARY_PARSING_READ_VALUE_HH
#define LIBBIO_BINARY_PARSING_READ_VALUE_HH

#include <algorithm>						// std::min
#include <array>
#include <cstring>							// memcpy
#include <exception>
#include <libbio/binary_parsing/endian.hh>
#include <libbio/binary_parsing/range.hh>
#include <libbio/type_traits.hh>
#include <span>
#include <string>
#include <vector>


namespace libbio::binary_parsing {
	
	// FIXME: Implement for other ranges than pairs of pointers.
	template <typename t_dst>
	requires is_trivial_rcvr_v <t_dst>
	inline bool copy_at_most(range &src, t_dst &dst)
	{
		if (! (src.it + sizeof(t_dst) <= src.end))
			return false;
		
		memcpy(&dst, src.it, sizeof(t_dst));
		src.it += sizeof(t_dst);
		return true;
	}
	
	// Overload for copying byte strings.
	inline bool copy_at_most(range &src, char *dst, std::size_t const size)
	{
		if (! (src.it + size <= src.end))
			return false;
		
		memcpy(dst, src.it, size);
		src.it += size;
		return true;
	}
	
	
	template <endian t_order, typename t_dst>
	requires is_integral_rcvr_v <t_dst>
	void read_value(range &rr, t_dst &dst)
	{
		static_assert(1 == sizeof(t_dst) || t_order != endian::unspecified);
		
		if (!copy_at_most(rr, dst))
			throw std::runtime_error("Unable to read expected number of bytes from the input");
		
		if constexpr (1 < sizeof(t_dst))
			boost::endian::conditional_reverse_inplace <detail::to_boost_endian_order(t_order), boost::endian::order::native>(dst);
	}
	
	
	template <endian t_order, typename t_dst>
	requires is_floating_point_rcvr_v <t_dst>
	void read_value(range &rr, t_dst &dst)
	{
		// Ignore order.
		if (!copy_at_most(rr, dst))
			throw std::runtime_error("Unable to read expected number of bytes from the input");
	}
	
	
	template <endian>
	void read_value(range &rr, std::vector <char> &dst)
	{
		if (!copy_at_most(rr, dst.data(), dst.size()))
			throw std::runtime_error("Unable to read expected number of bytes from the input");
	}
	
	
	template <endian>
	void read_value(range &rr, std::string &dst)
	{
		if (!copy_at_most(rr, dst.data(), dst.size()))
			throw std::runtime_error("Unable to read expected number of bytes from the input");
	}
	
	
	template <std::size_t t_size>
	void read_value(range &rr, std::array <char, t_size> &dst)
	{
		if (!copy_at_most(rr, dst.data(), t_size))
			throw std::runtime_error("Unable to read expected number of bytes from the input");
	}
	
	
	inline void read_zero_terminated_string(range &rr, std::string &dst)
	{
		std::size_t const bytes_left(rr.end - rr.it);
		if (!bytes_left)
			throw std::runtime_error("Unable to read expected number of bytes from the input");
		
		auto read_amt(std::min(bytes_left, dst.capacity() ?: 16));
		dst.resize(read_amt);
		char *dst_(dst.data());
		while (true)
		{
			auto const * const res(static_cast <char const *>(memccpy(dst_, rr.it, 0, read_amt)));
			if (res)
			{
				auto const total_length(res - dst.data() - 1);
				auto const did_read(res - dst_);
				rr.it += did_read;
				dst.resize(total_length);
				break;
			}
			
			// Did not yet find the NUL character.
			rr.it += read_amt;
			auto const bytes_left(rr.end - rr.it);
			auto const cur_size(dst.size());
			dst.resize(std::min(2 * cur_size, cur_size + bytes_left));
			dst_ = dst.data() + cur_size;
		}
	}
	
	
	template <typename t_type, endian t_order>
	t_type take(range &rr)
	{
		t_type retval{};
		read_value <t_order>(rr, retval);
		return retval;
	}
	
	
	template <typename t_type>
	std::span <t_type const> take_bytes(range &rr, std::size_t const size)
	{
		if (rr.end < rr.it + size)
			throw std::runtime_error("Unable to read expected number of bytes from the input");
		
		std::span <t_type const> retval(reinterpret_cast <t_type const *>(rr.it), size);
		rr.it += size;
		return retval;
	}
	
	
	template <std::size_t t_size, typename t_type = std::byte>
	std::span <t_type const, t_size> take_bytes(range &rr)
	{
		if (rr.end < rr.it + t_size)
			throw std::runtime_error("Unable to read expected number of bytes from the input");
		
		std::span <t_type const, t_size> retval(reinterpret_cast <t_type const *>(rr.it), t_size);
		rr.it += t_size;
		return retval;
	}
	
	
	std::span <char const> take_bytes(range &rr, std::size_t const size) { return take_bytes <char>(rr, size); }
}

#endif

/*
 * Copyright (c) 2020 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_SUBFIELD_COPY_VALUE_HH
#define LIBBIO_VCF_SUBFIELD_COPY_VALUE_HH

#include <vector>


namespace libbio::vcf::detail {
	
	template <typename t_src, typename t_dst>
	void copy_vector(std::vector <t_src> const &src, std::vector <t_dst> &dst);
	
	
	template <typename t_src, typename t_dst>
	struct copy_value
	{
		copy_value(t_src const &src, t_dst &dst)
		{
			dst = src;
		}
	};
	
	
	template <typename t_value>
	struct copy_value <std::vector <t_value>, std::vector <t_value>>
	{
		copy_value(std::vector <t_value> const &src, std::vector <t_value> &dst)
		{
			dst = src;
		}
	};
	
	
	template <typename t_src, typename t_dst>
	struct copy_value <std::vector <t_src>, std::vector <t_dst>>
	{
		copy_value(std::vector <t_src> const &src, std::vector <t_dst> &dst)
		{
			copy_vector(src, dst);
		}
	};
	
	
	template <typename t_src, typename t_dst>
	void copy_vector(std::vector <t_src> const &src, std::vector <t_dst> &dst)
	{
		auto const size(src.size());
		dst.resize(size);
		for (std::size_t i(0); i < size; ++i)
			copy_value(src[i], dst[i]);
	}
}

#endif

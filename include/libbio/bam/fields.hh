/*
 * Copyright (c) 2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_BAM_FIELDS_HH
#define LIBBIO_BAM_FIELDS_HH

#include <array>
#include <bit>									// std::endian
#include <boost/endian.hpp>
#include <cstddef>
#include <cstdint>
#include <libbio/binary_parsing/data_member.hh>
#include <libbio/binary_parsing/endian.hh>
#include <libbio/binary_parsing/field.hh>
#include <libbio/binary_parsing/range.hh>
#include <libbio/binary_parsing/read_value.hh>
#include <libbio/sam/cigar.hh>
#include <libbio/sam/optional_field.hh>
#include <libbio/sam/record.hh>
#include <libbio/sam/tag.hh>
#include <stdexcept>
#include <string>
#include <vector>


namespace libbio::bam::fields::detail {

	void read_hex_string(binary_parsing::range &range, std::vector <std::byte> &dst);


	// For simplifying the required friend declaration in sam::optional_field.
	struct optional_helper
	{
		template <typename t_dst_type, typename t_type>
		static inline void add_value(sam::optional_field &of, sam::tag_type const tag_id, t_type &&val) { of.template add_value <t_dst_type>(tag_id, std::forward <t_type>(val)); }

		template <typename t_type>
		static inline decltype(auto) start_array(sam::optional_field &of, sam::tag_type const tag_id) { return of.start_array <t_type>(tag_id); }

		static inline std::string &start_string(sam::optional_field &of, sam::tag_type const tag_id) { return of.start_string(tag_id); }
	};


	template <binary_parsing::endian t_order, typename t_type, typename t_dst_type = t_type, typename t_dst>
	inline void take_and_add_value(sam::tag_type const tag_id, binary_parsing::range &range, t_dst &dst)
	{
		optional_helper::add_value <t_dst_type>(dst, tag_id, binary_parsing::take <t_type, t_order>(range));
	}


	template <binary_parsing::endian t_order, typename t_type, typename t_dst_type = t_type>
	void read_array(sam::tag_type const tag_id, binary_parsing::range &range, sam::optional_field &dst)
	{
		auto &dst_(detail::optional_helper::start_array <t_dst_type>(dst, tag_id));
		dst_.clear();
		auto const size(binary_parsing::take <std::uint32_t, t_order>(range));

		// We can use memcpy if the destination type matches the source and, in case of integral types, the endian order matches.
		if constexpr (std::is_same_v <t_type, t_dst_type> && ((!std::is_integral_v <t_type>) || std::endian::native == std::endian::little))
		{
			dst_.resize(size);
			binary_parsing::copy_at_most(range, reinterpret_cast <char *>(dst_.data()), size * sizeof(t_dst_type));
		}
		else
		{
			for (std::uint32_t i(0); i < size; ++i)
			{
				auto val(binary_parsing::take <t_type, t_order>(range));
				if constexpr (std::is_integral_v <t_type>)
					boost::endian::conditional_reverse_inplace <binary_parsing::detail::to_boost_endian_order(t_order), boost::endian::order::native>(val);
				dst_.push_back(val);
			}
		}
	}
}


namespace libbio::bam::fields {

	template <binary_parsing::data_member t_mem>
	struct cigar : public binary_parsing::field_ <t_mem>
	{
		template <binary_parsing::endian t_order>
		void read_value(binary_parsing::range &rr, std::vector <sam::cigar_run> &dst) const;
	};


	template <binary_parsing::data_member t_mem>
	struct seq : public binary_parsing::field_ <t_mem>
	{
		// From SAMv1 § 4.2
		constexpr static std::array const mapping{'=', 'A', 'C', 'M', 'G', 'R', 'S', 'V', 'T', 'W', 'Y', 'H', 'K', 'D', 'B', 'N'};

		template <binary_parsing::endian t_order>
		void read_value(binary_parsing::range &rr, sam::record::sequence_type &dst) const;
	};


	template <binary_parsing::data_member t_mem>
	struct qual : public binary_parsing::field_ <t_mem>
	{
		template <binary_parsing::endian t_order>
		void read_value(binary_parsing::range &rr, sam::record::qual_type &dst) const;
	};


	template <binary_parsing::data_member t_mem>
	struct optional : public binary_parsing::field_ <t_mem>
	{
		template <binary_parsing::endian t_order>
		void read_value(binary_parsing::range &rr, sam::optional_field &dst) const;
	};


	template <binary_parsing::data_member t_mem>
	template <binary_parsing::endian t_order>
	void cigar <t_mem>::read_value(binary_parsing::range &rr, std::vector <sam::cigar_run> &dst) const
	{
		for (auto &run : dst)
		{
			std::uint32_t rep{};
			binary_parsing::read_value <t_order>(rr, rep);

			auto const count(rep >> 4);
			auto const op(rep & 0xf);

			if (! (0 <= op && op <= 8))
				throw std::runtime_error("Unexpected CIGAR operation number");

			run.assign(sam::cigar_run::count_type{count});
			run.assign(sam::cigar_operation(op)); // The order of the operations are the same as in the BAM format, i.e. MIDNSHP=X.
		}
	}


	template <binary_parsing::data_member t_mem>
	template <binary_parsing::endian t_order>
	void seq <t_mem>::read_value(binary_parsing::range &rr, sam::record::sequence_type &dst) const
	{
		auto const size(dst.size() / 2); // Handle all except the final value.
		for (std::size_t i(0); i < size; ++i)
		{
			std::uint8_t rep{};
			binary_parsing::read_value <t_order>(rr, rep);

			auto const first(rep >> 4);
			auto const second(rep & 0xf);

			dst[i * 2] = mapping[first];
			dst[i * 2 + 1] = mapping[second];
		}

		if (dst.size() & 0x1)
		{
			std::uint8_t rep{};
			binary_parsing::read_value <t_order>(rr, rep);
			dst.back() = mapping[rep >> 4];
		}
	}


	template <binary_parsing::data_member t_mem>
	template <binary_parsing::endian t_order>
	void qual <t_mem>::read_value(binary_parsing::range &rr, sam::record::qual_type &dst) const
	{
		binary_parsing::read_value <t_order>(rr, dst);
		if (dst.empty())
			return;

		if (0xff == std::bit_cast <unsigned char>(dst.front()))
		{
			dst.clear();
			return;
		}

		for (auto &val : dst)
			val += 33;
	}


	template <binary_parsing::data_member t_mem>
	template <binary_parsing::endian t_order>
	void optional <t_mem>::read_value(binary_parsing::range &rr, sam::optional_field &dst) const
	{
		auto const sp(binary_parsing::take_bytes <3, char>(rr));
		auto const tag_id(sam::to_tag_(sp));
		auto const value_type_code(sp[2]);

		switch (value_type_code)
		{
			case 'A':	detail::take_and_add_value <t_order, char>(tag_id, rr, dst);			break;
			case 'c':	detail::take_and_add_value <t_order, std::int8_t>(tag_id, rr, dst);		break;
			case 'C':	detail::take_and_add_value <t_order, std::uint8_t>(tag_id, rr, dst);	break;
			case 's':	detail::take_and_add_value <t_order, std::int16_t>(tag_id, rr, dst);	break;
			case 'S':	detail::take_and_add_value <t_order, std::uint16_t>(tag_id, rr, dst);	break;
			case 'i':	detail::take_and_add_value <t_order, std::int32_t>(tag_id, rr, dst);	break;
			case 'I':	detail::take_and_add_value <t_order, std::uint32_t>(tag_id, rr, dst);	break;
			case 'f':	detail::take_and_add_value <t_order, float, sam::optional_field::floating_point_type>(tag_id, rr, dst); break;
			case 'Z':	// std::string
			{
				auto &dst_(detail::optional_helper::start_string(dst, tag_id));
				binary_parsing::read_zero_terminated_string(rr, dst_);
				break;
			}
			case 'H':	// Hex array
			{
				auto &dst_(detail::optional_helper::start_array <std::byte>(dst, tag_id));
				detail::read_hex_string(rr, dst_);
			}
			case 'B':	// Byte or word array
			{
				auto const type_code(binary_parsing::take <char, t_order>(rr));
				switch (type_code)
				{
					case 'c':	detail::read_array <t_order, std::int8_t>(tag_id, rr, dst);		break;
					case 'C':	detail::read_array <t_order, std::uint8_t>(tag_id, rr, dst);	break;
					case 's':	detail::read_array <t_order, std::int16_t>(tag_id, rr, dst);	break;
					case 'S':	detail::read_array <t_order, std::uint16_t>(tag_id, rr, dst);	break;
					case 'i':	detail::read_array <t_order, std::int32_t>(tag_id, rr, dst);	break;
					case 'I':	detail::read_array <t_order, std::uint32_t>(tag_id, rr, dst);	break;
					case 'f':	detail::read_array <t_order, float, sam::optional_field::floating_point_type>(tag_id, rr, dst); break;
					default:	throw std::runtime_error("Unexpected array type");
				}
			}

			default: throw std::runtime_error("Unexpected tag type");
		}
	}
}

#endif

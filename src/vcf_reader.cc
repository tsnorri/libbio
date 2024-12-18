/*
 * Copyright (c) 2017-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <libbio/assert.hh>
#include <libbio/utility.hh>
#include <libbio/vcf/constants.hh>
#include <libbio/vcf/metadata.hh>
#include <libbio/vcf/subfield.hh>
#include <libbio/vcf/variant.hh>
#include <libbio/vcf/vcf_reader.hh>
#include <libbio/vcf/vcf_reader_support.hh>
#include <range/v3/view/enumerate.hpp>
#include <range/v3/view/zip.hpp>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>


namespace libbio::vcf::detail {

	reader_default_delegate	g_vcf_reader_default_delegate;
	variant_no_op_validator	g_vcf_reader_default_variant_validator;


	template <typename t_base>
	struct placeholder_field_helper
	{
	};

	template <>
	struct placeholder_field_helper <info_field_base>
	{
		static info_field_base &get_placeholder() { return reader_support::get_instance().get_info_field_placeholder(); }
	};

	template <>
	struct placeholder_field_helper <genotype_field_base>
	{
		static genotype_field_base &get_placeholder() { return reader_support::get_instance().get_genotype_field_placeholder(); }
	};


	class metadata_setup_helper
	{
	public:
		template <template <vcf::metadata_value_type, std::int32_t> typename t_field_tpl, vcf::metadata_value_type t_field_type, typename t_field_base_class>
		static auto instantiate_field(vcf::metadata_formatted_field const &meta) -> t_field_base_class *
		{
			switch (meta.get_number())
			{
				// Assume that one is a rather typical case.
				case 1:
					return new t_field_tpl <t_field_type, 1>();

				case vcf::VCF_NUMBER_ONE_PER_ALTERNATE_ALLELE:
					return new t_field_tpl <t_field_type, vcf::VCF_NUMBER_ONE_PER_ALTERNATE_ALLELE>();

				case vcf::VCF_NUMBER_ONE_PER_ALLELE:
					return new t_field_tpl <t_field_type, vcf::VCF_NUMBER_ONE_PER_ALLELE>();

				case vcf::VCF_NUMBER_ONE_PER_GENOTYPE:
					return new t_field_tpl <t_field_type, vcf::VCF_NUMBER_ONE_PER_GENOTYPE>();

				case vcf::VCF_NUMBER_UNKNOWN:
				case vcf::VCF_NUMBER_DETERMINED_AT_RUNTIME:
				default:
					return new t_field_tpl <t_field_type, vcf::VCF_NUMBER_DETERMINED_AT_RUNTIME>();
			}
		}


		template <template <vcf::metadata_value_type, std::int32_t> typename t_field_tpl, typename t_meta, typename t_key, typename t_field_map>
		static auto find_or_add_field(
			t_meta &meta,
			t_key const &key,
			t_field_map &map,
			vcf::reader_delegate &delegate
		) -> typename t_field_map::mapped_type::element_type &
		{
			auto const res(map.try_emplace(key));
			auto &field_ptr(res.first->second);
			if (!res.second)
			{
				// Did not add.
				auto &field(*field_ptr);
				if ((
					field.metadata_value_type() == meta.get_value_type() &&
					field.number() == meta.get_number() // FIXME: We could take the special values into account when comparing.
				) || !delegate.vcf_reader_should_replace_non_matching_subfield(key, field, meta))
				{
					field.m_metadata = &meta;
					return field;
				}
			}

			// Did add or the delegate asks to replace.
			typedef typename t_field_map::mapped_type::element_type field_base_class;
			field_base_class *ptr{};
			switch (meta.get_value_type())
			{
				case vcf::metadata_value_type::INTEGER:
					ptr = instantiate_field <t_field_tpl, vcf::metadata_value_type::INTEGER, field_base_class>(meta);
					break;

				case vcf::metadata_value_type::FLOAT:
					ptr = instantiate_field <t_field_tpl, vcf::metadata_value_type::FLOAT, field_base_class>(meta);
					break;

				case vcf::metadata_value_type::CHARACTER:
					ptr = instantiate_field <t_field_tpl, vcf::metadata_value_type::CHARACTER, field_base_class>(meta);
					break;

				case vcf::metadata_value_type::STRING:
					ptr = instantiate_field <t_field_tpl, vcf::metadata_value_type::STRING, field_base_class>(meta);
					break;

				case vcf::metadata_value_type::FLAG:
					ptr = new t_field_tpl <vcf::metadata_value_type::FLAG, 0>();
					break;

				case vcf::metadata_value_type::UNKNOWN:
				case vcf::metadata_value_type::NOT_PROCESSED:
				default:
					ptr = vcf::detail::placeholder_field_helper <field_base_class>::get_placeholder().clone();
					break;
			}

			libbio_always_assert(ptr);
			field_ptr.reset(ptr);
			meta.check_field(*ptr);
			ptr->m_metadata = &meta;
			return *field_ptr;
		}


		template <typename t_field>
		static std::pair <std::uint16_t, std::uint16_t> sort_and_assign_field_offsets(std::vector <t_field *> &field_vec)
		{
			if (!field_vec.empty())
			{
				std::sort(field_vec.begin(), field_vec.end(), [](auto const lhs, auto const rhs) -> bool {
					auto const la(lhs->alignment());
					auto const ls(lhs->byte_size());
					auto const ra(rhs->alignment());
					auto const rs(rhs->byte_size());

					return std::tie(la, ls) > std::tie(ra, rs);
				});

				// Determine and assign the offsets.
				std::uint16_t next_offset(0);
				std::uint16_t max_alignment(1);
				for (auto &field : field_vec)
				{
					auto const alignment(field->alignment());
					libbio_assert_lt(0, alignment);
					max_alignment = std::max(alignment, max_alignment);
					if (0 != next_offset % alignment)
						next_offset = alignment * (next_offset / alignment + 1);

					field->set_offset(next_offset);
					next_offset += field->byte_size();
				}

				return {next_offset, max_alignment};
			}

			return {0, 1};
		}
	};
}


namespace libbio::vcf {

	void reader::report_unexpected_character(char const *current_character, std::size_t const pos, int const current_state, bool const in_header)
	{
		// FIXME: wrap everything to the exception.

		if (in_header)
			std::cerr << "Unexpected character in VCF header ";
		else
			std::cerr << "Unexpected character ";

		std::cerr
		<< '\'' << (*current_character) << "' (" << +(*current_character) << ") at " << m_lineno << ':' << pos
		<< ", state " << current_state << '.' << std::endl;

		auto const start(m_fsm.p);
		auto const end(m_fsm.pe - m_fsm.p < 128 ? m_fsm.pe - m_fsm.p : 128);
		std::string_view buffer_end(start, end);
		std::cerr
		<< "** Last 128 charcters from the buffer:" << std::endl
		<< buffer_end << std::endl;

		throw std::runtime_error("Unexpected character");
	}


	void reader::skip_to_next_nl()
	{
		std::string_view sv(m_fsm.p, m_fsm.pe - m_fsm.p);
		auto const pos(sv.find('\n'));
		libbio_always_assert_msg(std::string_view::npos != pos, "Unable to find the next newline");
		m_fsm.p += pos;
	}


	// TODO: Implement seeking. Currently this is possible (to some extent) by using a MMAP data source and a separate reader.
#if 0
	// Seek to the beginning of the records.
	void reader::reset()
	{
		libbio_assert(m_input);
		m_input->reset_to_first_variant_offset();
		m_lineno = m_input->last_header_lineno();
		m_fsm.eof = nullptr;
		m_current_line_or_buffer_start = nullptr;
		m_counter = 0;
		m_variant_index = 0;

		m_current_format->m_fields_by_identifier.clear();
		m_current_format_vec.clear();
	}
#endif


	// Return the 1-based number of the given sample.
	std::size_t reader::sample_no(std::string const &sample_name) const
	{
		auto const it(m_sample_indices_by_name.find(sample_name));
		if (it == m_sample_indices_by_name.cend())
			return 0;
		return it->second;
	}


	void reader::fill_buffer()
	{
		libbio_assert(m_input);
		if (m_fsm.p == m_fsm.pe && (m_fsm.p != m_fsm.eof || m_fsm.p == nullptr))
		{
			m_input->fill_buffer(*this);
			m_current_line_or_buffer_start = m_input->buffer_start();
		}
	}


	void reader::associate_metadata_with_field_descriptions()
	{
		m_info_fields_in_headers.reserve(m_metadata.m_info.size());
		m_current_record_info_fields.reserve(m_metadata.m_info.size());

		for (auto &[key, meta] : m_metadata.m_info)
		{
			auto &info_field(detail::metadata_setup_helper::find_or_add_field <info_field>(meta, key, m_info_fields, *m_delegate));
			m_info_fields_in_headers.emplace_back(&info_field);
		}

		for (auto &[key, meta] : m_metadata.m_format)
			auto &genotype_field(detail::metadata_setup_helper::find_or_add_field <genotype_field>(meta, key, m_genotype_fields, *m_delegate));
	}


	std::pair <std::uint16_t, std::uint16_t> reader::assign_info_field_offsets()
	{
		// Sort the info fields by alignment requirement and size, then assign offsets.
		for (auto const &[key, field_ptr] : m_info_fields)
			field_ptr->set_offset(subfield_base::INVALID_OFFSET);

		return detail::metadata_setup_helper::sort_and_assign_field_offsets(m_info_fields_in_headers);
	}


	void reader::parse_format(std::string_view const &new_format_sv)
	{
		auto *new_format(m_current_format->new_instance());
		libbio_assert_eq(typeid(*m_current_format), typeid(*new_format));
		m_current_format.reset(new_format);
		m_current_format_vec.clear();

		std::size_t start_pos(0);
		while (true)
		{
			auto const end_pos(new_format_sv.find_first_of(':', start_pos));
			auto const length(end_pos == std::string_view::npos ? end_pos : end_pos - start_pos);
			auto const format_key(new_format_sv.substr(start_pos, length)); // npos is valid for end_pos.
			auto const it(m_genotype_fields.find(format_key));
			if (m_genotype_fields.end() == it)
				throw std::runtime_error("Unexpected key in FORMAT");
			if (!it->second->m_metadata)
				throw std::runtime_error("FORMAT field not declared in the VCF header");

			auto const res(new_format->m_fields_by_identifier.emplace(
				std::piecewise_construct,
				std::forward_as_tuple(format_key),
				std::forward_as_tuple(it->second->clone())
			));
			if (!res.second)
				throw std::runtime_error("Format key already in place");
			m_current_format_vec.emplace_back(res.first->second.get());

			if (std::string_view::npos == end_pos)
				break;

			start_pos = 1 + end_pos;
		}
	}


	std::pair <std::uint16_t, std::uint16_t> reader::assign_format_field_indices_and_offsets()
	{
		// Recalculate the offsets. This also has the effect of making variants copied earlier unreadable,
		// unless the subfield descriptors have been copied by the client.
		for (auto const &[key, field_ptr] : m_genotype_fields)
			field_ptr->set_offset(subfield_base::INVALID_OFFSET);

		auto format_vec(m_current_format_vec); // Copy b.c. the field order needs to be preserved.
		auto const retval(detail::metadata_setup_helper::sort_and_assign_field_offsets(format_vec));
		for (auto const &[idx, field_ptr] : ranges::view::enumerate(format_vec))
			field_ptr->set_index(idx);
		return retval;
	}
}

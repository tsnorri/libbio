/*
 * Copyright (c) 2017-2020 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <libbio/utility.hh>
#include <libbio/vcf/subfield.hh>
#include <libbio/vcf/variant.hh>
#include <libbio/vcf/vcf_reader.hh>
#include <libbio/vcf/vcf_reader_support.hh>
#include <range/v3/view/enumerate.hpp>
#include <range/v3/view/zip.hpp>


namespace libbio::vcf::detail {

	reader_default_delegate  g_vcf_reader_default_delegate;

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
}


namespace libbio::vcf {
	
	void reader::report_unexpected_character(char const *current_character, std::size_t const pos, int const current_state, bool const in_header)
	{
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

		abort();
	}
	
	
	void reader::skip_to_next_nl()
	{
		std::string_view sv(m_fsm.p, m_fsm.pe - m_fsm.p);
		auto const pos(sv.find('\n'));
		libbio_always_assert_msg(std::string_view::npos != pos, "Unable to find the next newline");
		m_fsm.p += pos;
	}
	
	
	// TODO: implement seeking.
#if 0
	// Seek to the beginning of the records.
	void reader::reset()
	{
		libbio_assert(m_input);
		m_input->reset_to_first_variant_offset();
		m_lineno = m_input->last_header_lineno();
		m_fsm.eof = nullptr;
		m_current_line_start = nullptr;
		m_counter = 0;
		m_variant_index = 0;
		
		m_current_format->m_fields_by_identifier.clear();
		m_current_format_vec.clear();
	}
#endif
	
	
	// Return the 1-based number of the given sample.
	std::size_t reader::sample_no(std::string const &sample_name) const
	{
		auto const it(m_sample_names.find(sample_name));
		if (it == m_sample_names.cend())
			return 0;
		return it->second;
	}
	
	
	void reader::fill_buffer()
	{
		libbio_assert(m_input);
		if (m_fsm.p == m_fsm.pe && (m_fsm.p != m_fsm.eof || m_fsm.p == nullptr))
			m_input->fill_buffer(*this);
	}
	
	
	template <template <std::int32_t, metadata_value_type> typename t_field_tpl, metadata_value_type t_field_type, typename t_field_base_class>
	auto reader::instantiate_field(metadata_formatted_field const &meta) -> t_field_base_class *
	{
		switch (meta.get_number())
		{
			// Assume that one is a rather typical case.
			case 1:
				return new t_field_tpl <1, t_field_type>();

			case VCF_NUMBER_ONE_PER_ALTERNATE_ALLELE:
				return new t_field_tpl <VCF_NUMBER_ONE_PER_ALTERNATE_ALLELE, t_field_type>();
				
			case VCF_NUMBER_ONE_PER_ALLELE:
				return new t_field_tpl <VCF_NUMBER_ONE_PER_ALLELE, t_field_type>();
				
			case VCF_NUMBER_ONE_PER_GENOTYPE:
				return new t_field_tpl <VCF_NUMBER_ONE_PER_GENOTYPE, t_field_type>();
			
			case VCF_NUMBER_UNKNOWN:
			case VCF_NUMBER_DETERMINED_AT_RUNTIME:
			default:
				return new t_field_tpl <VCF_NUMBER_DETERMINED_AT_RUNTIME, t_field_type>();
		}
	}
	
	
	template <template <std::int32_t, metadata_value_type> typename t_field_tpl, typename t_key, typename t_field_map>
	auto reader::find_or_add_field(metadata_formatted_field const &meta, t_key const &key, t_field_map &map, bool &did_add) -> typename t_field_map::mapped_type::element_type &
	{
		auto const it(map.find(key));
		if (map.end() != it)
		{
			did_add = false;
			return *it->second;
		}
		
		typedef typename t_field_map::mapped_type::element_type field_base_class;
		field_base_class *val{};
		switch (meta.get_value_type())
		{
			case metadata_value_type::INTEGER:
				val = instantiate_field <t_field_tpl, metadata_value_type::INTEGER, field_base_class>(meta);
				break;
				
			case metadata_value_type::FLOAT:
				val = instantiate_field <t_field_tpl, metadata_value_type::FLOAT, field_base_class>(meta);
				break;
			
			case metadata_value_type::CHARACTER:
				val = instantiate_field <t_field_tpl, metadata_value_type::CHARACTER, field_base_class>(meta);
				break;
				
			case metadata_value_type::STRING:
				val = instantiate_field <t_field_tpl, metadata_value_type::STRING, field_base_class>(meta);
				break;
			
			case metadata_value_type::FLAG:
				val = new t_field_tpl <0, metadata_value_type::FLAG>();
				break;
			
			case metadata_value_type::UNKNOWN:
			case metadata_value_type::NOT_PROCESSED:
			default:
				val = detail::placeholder_field_helper <field_base_class>::get_placeholder().clone();
				break;
		}
		
		auto const res(map.emplace(
			std::piecewise_construct,
			std::forward_as_tuple(key),
			std::forward_as_tuple(val)
		));
		
		libbio_always_assert(res.second);
		did_add = true;
		return *res.first->second;
	}
	
	
	void reader::associate_metadata_with_field_descriptions()
	{
		bool did_add(false);
		for (auto &[key, meta] : m_metadata.m_info)
		{
			auto &info_field(find_or_add_field <info_field>(meta, key, m_info_fields, did_add));
			if (did_add)
				meta.check_field(info_field);
			info_field.m_metadata = &meta;
			m_info_fields_in_headers.emplace_back(&info_field);
		}
		
		for (auto &[key, meta] : m_metadata.m_format)
		{
			auto &genotype_field(find_or_add_field <genotype_field>(meta, key, m_genotype_fields, did_add));
			if (did_add)
				meta.check_field(genotype_field);
			genotype_field.m_metadata = &meta;
		}
	}
	
	
	template <typename t_field>
	std::pair <std::uint16_t, std::uint16_t> reader::sort_and_assign_field_offsets(std::vector <t_field *> &field_vec) const
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
			
			return std::make_pair(next_offset, max_alignment);
		}
		
		return std::make_pair(0, 1);
	}
	
	
	std::pair <std::uint16_t, std::uint16_t> reader::assign_info_field_offsets()
	{
		// Sort the info fields by alignment requirement and size, then assign offsets.
		for (auto const &[key, field_ptr] : m_info_fields)
			field_ptr->set_offset(subfield_base::INVALID_OFFSET);
		
		return sort_and_assign_field_offsets(m_info_fields_in_headers);
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
		auto const retval(sort_and_assign_field_offsets(format_vec));
		for (auto const &[idx, field_ptr] : ranges::view::enumerate(format_vec))
			field_ptr->set_index(idx);
		return retval;
	}
}

/*
 * Copyright (c) 2020-2025 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_VARIANT_FORMATTED_VARIANT_DEF_HH
#define LIBBIO_VCF_VARIANT_FORMATTED_VARIANT_DEF_HH

#include <cstdint>
#include <libbio/assert.hh>
#include <libbio/vcf/variant_format.hh>
#include <libbio/vcf/variant/variant_tpl.hh>
#include <cstddef>
#include <libbio/vcf/subfield/info_field_base_def.hh>
#include <libbio/vcf/variant/formatted_variant_decl.hh>
#include <libbio/vcf/vcf_reader_decl.hh>
#include <range/v3/view/zip.hpp>
#include <utility>


namespace libbio::vcf::detail {

	variant_format_ptr const &transient_variant_format_access::get_format_ptr(reader const &vcf_reader) const
	{
		return vcf_reader.get_variant_format_ptr();
	}


	variant_format_access::variant_format_access(reader const &vcf_reader):
		m_format(vcf_reader.get_variant_format_ptr())
	{
	}


	variant_format_access::variant_format_access(reader const &vcf_reader, transient_variant_format_access const &other):
		m_format(other.get_format_ptr(vcf_reader))
	{
	}
}


namespace libbio::vcf {

	// Constructor.
	template <typename t_string, typename t_format_access>
	formatted_variant <t_string, t_format_access>::formatted_variant(
		reader &reader,
		std::size_t const sample_count,
		std::size_t const info_size,
		std::size_t const info_alignment
	):
		libbio::vcf::variant_tpl <t_string>(reader, sample_count, info_size, info_alignment),
		t_format_access(reader)
	{
		initialize_info();
		initialize_samples();
	}


	// Copy constructors.
	template <typename t_string, typename t_format_access>
	formatted_variant <t_string, t_format_access>::formatted_variant(formatted_variant const &other):
		variant_tpl <t_string>(other),
		t_format_access(other)
	{
		this->finish_copy(other, true, true);
	}

	template <typename t_string, typename t_format_access>
	template <typename t_other_string, typename t_other_format_access>
	formatted_variant <t_string, t_format_access>::formatted_variant(formatted_variant <t_other_string, t_other_format_access> const &other):
		variant_tpl <t_string>(other),
		t_format_access(*this->m_reader, other) // Copy the format from a non-transient variant or get one from the reader in case of a transient variant.
	{
		this->finish_copy(other, true, true);
	}


	// Copy assignment operators.
	template <typename t_string, typename t_format_access>
	auto formatted_variant <t_string, t_format_access>::operator=(formatted_variant const &other) & -> formatted_variant &
	{
		if (this != &other)
		{
			auto const [should_init_info, should_init_samples] = this->prepare_for_copy(other);
			variant_tpl <t_string>::operator=(other);
			t_format_access::operator=(other);
			this->finish_copy(other, should_init_info, should_init_samples);
		}
		return *this;
	}


	template <typename t_string, typename t_format_access>
	template <typename t_other_string, typename t_other_format_access>
	auto formatted_variant <t_string, t_format_access>::operator=(formatted_variant <t_other_string, t_other_format_access> const &other) & -> formatted_variant &
	{
		auto const vfptr(other.m_reader->get_variant_format_ptr());
		libbio_assert(vfptr);

		auto const [should_init_info, should_init_samples] = this->prepare_for_copy(other);
		variant_tpl <t_string>::operator=(other);
		this->set_format_ptr(vfptr);
		this->finish_copy(other, should_init_info, should_init_samples);

		return *this;
	}


	// Destructor.
	template <typename t_string, typename t_format_access>
	formatted_variant <t_string, t_format_access>::~formatted_variant()
	{
		if (this->m_reader)
		{
			deinitialize_info();
			deinitialize_samples();
		}
	}


	template <typename t_string, typename t_format_access>
	variant_format_ptr const &formatted_variant <t_string, t_format_access>::get_format_ptr() const
	{
		return t_format_access::get_format_ptr(*this->m_reader);
	}


	template <typename t_string, typename t_format_access>
	variant_format const &formatted_variant <t_string, t_format_access>::get_format() const
	{
		return *get_format_ptr();
	}


	template <typename t_string, typename t_format_access>
	void formatted_variant <t_string, t_format_access>::initialize_info()
	{
		// Info.
		auto *bytes(this->m_info.get());
		if (bytes)
		{
			for (auto const *field_ptr : this->m_reader->m_info_fields_in_headers)
			{
				libbio_assert_lte(field_ptr->get_offset() + field_ptr->byte_size(), this->m_info.size());
				field_ptr->construct_ds(*this, bytes, 16); // Assume that the REF + ALT count is at most 16.
			}
		}
	}


	template <typename t_string, typename t_format_access>
	void formatted_variant <t_string, t_format_access>::deinitialize_info()
	{
		// Info.
		auto *bytes(this->m_info.get());
		if (bytes)
		{
			for (auto const *field_ptr : this->m_reader->m_info_fields_in_headers)
				field_ptr->destruct_ds(*this, bytes);
		}
	}


	template <typename t_string, typename t_format_access>
	void formatted_variant <t_string, t_format_access>::initialize_samples()
	{
		// The samples are preallocated, see read_header().
		// Also the variant data has been preallocated either
		// in reserve_memory_for_samples_in_current_variant or by the copy constructor.
		auto const &fields_by_id(this->get_format().fields_by_identifier());
		for (auto &sample : this->m_samples)
		{
			auto &sample_data(sample.m_sample_data);
			auto *bytes(sample_data.get());
			if (bytes)
			{
				for (auto const &[key, field_ptr] : fields_by_id)
				{
					libbio_assert_lte(field_ptr->get_offset() + field_ptr->byte_size(), sample_data.size());
					field_ptr->construct_ds(sample, bytes, 16); // Assume that the REF + ALT count is at most 16. Vector’s emplace_back is used for adding values, though.
				}
			}
		}
	}


	template <typename t_string, typename t_format_access>
	void formatted_variant <t_string, t_format_access>::deinitialize_samples()
	{
		libbio_assert(this->m_reader);
		auto const &fields_by_id(this->get_format().fields_by_identifier());
		for (auto &sample : this->m_samples)
		{
			auto *bytes(sample.m_sample_data.get());
			if (bytes)
			{
				for (auto const &[key, field_ptr] : fields_by_id)
					field_ptr->destruct_ds(sample, bytes); // Assume that the REF + ALT count is at most 16.
			}
		}
	}


	template <typename t_string, typename t_format_access>
	void formatted_variant <t_string, t_format_access>::reserve_memory_for_samples(
		std::uint16_t const size,
		std::uint16_t const alignment,
		std::uint16_t const field_count
	)
	{
		// For now, the variant needs to be initialized with the correct sample count.
		if (this->m_samples.empty())
			return;

		auto const &first_sample(this->m_samples.front());
		auto const &first_sample_data(first_sample.m_sample_data);
		auto const &first_sample_assigned_fields(first_sample.m_assigned_genotype_fields);
		auto const prev_size(first_sample_data.size());
		auto const prev_alignment(first_sample_data.alignment());

		bool const size_changed(size && (prev_size < size || prev_alignment < alignment));
		bool const count_changed(first_sample_assigned_fields.size() != field_count);
		if (size_changed || count_changed)
		{
			for (auto &sample : this->m_samples)
			{
				if (size_changed)
					sample.m_sample_data.realloc(size, alignment);
				if (count_changed)
					sample.m_assigned_genotype_fields.resize(field_count, false);
			}
		}
	}


	template <typename t_string, typename t_format_access>
	template <typename t_other_string, typename t_other_format_access>
	std::pair <bool, bool> formatted_variant <t_string, t_format_access>::prepare_for_copy(
		formatted_variant <t_other_string, t_other_format_access> const &other
	)
	{
		if (this->m_reader && other.m_reader)
		{
			auto const *old_format(get_format_ptr().get());
			auto const *new_format(other.get_format_ptr().get());
			libbio_assert(old_format, "Variant format should not be nullptr if m_reader is set.");
			libbio_assert(new_format, "Variant format should not be nullptr if m_reader is set.");

			if (old_format == new_format || *old_format == *new_format)
				return std::pair <bool, bool>(false, false);

			// Formats do not match. Get rid of the old samples.
			deinitialize_samples();
			return std::pair <bool, bool>(false, true);
		}
		else if (this->m_reader)
		{
			// other is empty.
			libbio_assert(!other.m_reader);
			deinitialize_info();
			deinitialize_samples();
			return std::pair <bool, bool>(true, true);
		}
		else
		{
			libbio_assert(other.m_reader);
			return std::pair <bool, bool>(true, true);
		}
	}


	template <typename t_string, typename t_format_access>
	template <typename t_other_string, typename t_other_format_access>
	void formatted_variant <t_string, t_format_access>::finish_copy(
		formatted_variant <t_other_string, t_other_format_access> const &src,
		bool const should_initialize_info,
		bool const should_initialize_samples
	)
	{
		if (this->m_reader)
		{
			// Zeroed when copying.
			if (should_initialize_info)
				initialize_info();
			if (should_initialize_samples)
				initialize_samples();

			auto const &format(this->get_format());
			auto const &fields_by_id(format.fields_by_identifier());

			// Info.
			auto const *src_bytes(src.m_info.get());
			auto *dst_bytes(this->m_info.get());
			for (auto const *field_ptr : this->m_reader->m_info_fields_in_headers)
			{
				if (field_ptr->has_value(src))
					field_ptr->copy_ds(src, *this, src_bytes, dst_bytes);
			}

			// Samples.
			libbio_always_assert_eq(src.m_samples.size(), this->m_samples.size());
			for (auto const &[src_sample, dst_sample] : ranges::views::zip(src.m_samples, this->m_samples))
			{
				auto const	*src_bytes(src_sample.m_sample_data.get());
				auto		*dst_bytes(dst_sample.m_sample_data.get());

				for (auto const &[key, field_ptr] : format.fields_by_identifier())
					field_ptr->copy_ds(src_sample, dst_sample, src_bytes, dst_bytes);
			}
		}
	}
}

#endif

/*
 * Copyright (c) 2023 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_SAM_INPUT_RANGE_HH
#define LIBBIO_SAM_INPUT_RANGE_HH

#include <libbio/generic_parser.hh>
#include <libbio/file_handle.hh>
#include <vector>


namespace libbio::sam {
	
	struct input_range_base : public parsing::updatable_range_base <char const *>
	{
		using parsing::updatable_range_base <char const *>::updatable_range_base;
		virtual void prepare() = 0;
	};
	
	
	struct character_range final : public input_range_base
	{
		using input_range_base::input_range_base;
		
		explicit character_range(std::string_view const sv):
			input_range_base(sv.data(), sv.data() + sv.size())
		{
		}
		
		void prepare() override {}
		bool update() override { it = nullptr; sentinel = nullptr; return false; }
	};
	
	
	struct file_handle_input_range final : public input_range_base
	{
		file_handle			&fh;
		std::vector <char>	buffer;
		
		explicit file_handle_input_range(file_handle &fh_):
			input_range_base(nullptr, nullptr),
			fh(fh_)
		{
			buffer.resize(fh.io_op_blocksize(), 0);
		}
		
		void prepare() override { update(); }
		bool update() override;
	};
	
	
	struct file_handle_input_range_ final : public input_range_base
	{
		file_handle 		fh;
		std::vector <char>	buffer;
		
		explicit file_handle_input_range_(file_handle &&fh_):
			input_range_base(nullptr, nullptr),
			fh(std::move(fh_))
		{
			buffer.resize(fh.io_op_blocksize(), 0);
		}
		
		void prepare() override { update(); }
		bool update() override;
	};
}

#endif

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
	
	typedef parsing::updatable_range_base <char const *> input_range_base;
	
	
	struct file_handle_input_range final : public input_range_base
	{
		file_handle			&fh;
		std::vector <char>	buffer;
		
		file_handle_input_range(file_handle &fh_):
			input_range_base(nullptr, nullptr),
			fh(fh_)
		{
			buffer.resize(fh.io_op_blocksize(), 0);
		}
		
		void prepare() { update(); }
		bool update() override;
	};
}

#endif

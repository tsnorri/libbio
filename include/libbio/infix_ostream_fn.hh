/*
 * Copyright (c) 2017-2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_INFIX_OSTREAM_FN_HH
#define LIBBIO_INFIX_OSTREAM_FN_HH

#include <iostream>


namespace libbio {
	
	template <typename t_item>
	class infix_ostream_fn
	{
	protected:
		std::ostream	*m_ostream{nullptr};
		char const		m_delim{0};
		bool			m_first_item{true};
		
	public:
		infix_ostream_fn() = default;
		
		infix_ostream_fn(
			std::ostream &stream,
			char const delim
		):
			m_ostream(&stream),
			m_delim(delim)
		{
		}
		
		void operator()(t_item const &item);
	};
	
	
	template <typename t_item>
	void infix_ostream_fn <t_item>::operator()(t_item const &item)
	{
		if (!m_first_item)
			*m_ostream << m_delim;
		
		*m_ostream << item;
		m_first_item = false;
	}
}

#endif

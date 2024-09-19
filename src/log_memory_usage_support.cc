/*
 * Copyright (c) 2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <libbio/log_memory_usage.hh>


namespace libbio::memory_logger {
	
	std::ostream &operator<<(std::ostream &os, event::type const type)
	{
		switch (type)
		{
			case event::type::allocated_amount:
				os << "allocated_amount";
				break;
				
			case event::type::marker:
				os << "marker";
				break;
			
			case event::type::unknown:
			default:
				os << "unknown";
				break;
		}
		
		return os;
	}
	
	
	std::ostream &operator<<(std::ostream &os, event const &evt)
	{
		switch (evt.event_type())
		{
			case event::type::allocated_amount:
				os << "m:" << evt.event_data();
				break;
				
			case event::type::marker:
				os << "M:" << evt.event_data();
				break;
			
			case event::type::unknown:
			default:
				os << "unknown";
				break;
		}
		
		return os;
	}
	
	
	void event::output_record(std::ostream &os) const
	{
		os << event_type();
		os << '\t';
		os << event_data();
	}
}

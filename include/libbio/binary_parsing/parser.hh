/*
 * Copyright (c) 2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_BINARY_PARSING_PARSER_HH
#define LIBBIO_BINARY_PARSING_PARSER_HH

#include <libbio/binary_parsing/data_member.hh>
#include <libbio/binary_parsing/endian.hh>
#include <libbio/binary_parsing/field.hh>
#include <libbio/binary_parsing/range.hh>


namespace libbio::binary_parsing {

	struct parser_base
	{
		~parser_base() {}
		virtual void parse() = 0;
	};


	template <endian t_order, typename t_parser, typename t_target>
	class parser_proxy
	{
	public:
		constexpr static inline endian const byte_order{t_order};

	private:
		t_parser		*m_parser{};
		t_target		*m_target{};

	public:
		explicit parser_proxy(t_parser &parser, t_target &target):
			m_parser(&parser),
			m_target(&target)
		{
		}

		t_target &target() { return *m_target; }

		template <data_member t_mem, template <data_member> typename t_base = field_>
		void read_field() { detail::read_field <byte_order, t_mem, t_base>(m_parser->range(), target()); }

		template <typename t_iterable, typename t_fn>
		inline void for_(t_iterable &iterable, t_fn &&fn);
	};

	template <endian t_order, typename t_parser, typename t_target>
	auto make_parser_proxy(t_parser &pp, t_target &tt) { return parser_proxy <t_order, t_parser, t_target>(pp, tt); }
}


namespace libbio::binary_parsing::detail {

	template <endian t_order, typename t_parser, typename t_iterable, typename t_fn>
	inline void for_(t_parser &parser, t_iterable &iterable, t_fn &&fn)
	{
		for (auto &val : iterable)
		{
			auto pp(make_parser_proxy <t_order>(parser, val));
			fn(pp);
		}
	}
}


namespace libbio::binary_parsing {

	template <endian t_order, typename t_parser, typename t_target>
	template <typename t_iterable, typename t_fn>
	void parser_proxy <t_order, t_parser, t_target>::for_(t_iterable &iterable, t_fn &&fn)
	{
		detail::for_ <t_order>(*m_parser, iterable, std::forward <t_fn>(fn));
	}


	template <typename t_target, endian t_order>
	class parser_ : public parser_base
	{
	public:
		constexpr static inline endian const byte_order{t_order};

	protected:
		struct range	*m_range{};
		t_target		*m_target{};

	public:
		struct range &range() { return *m_range; }
		t_target &target() { return *m_target; }

	protected:
		template <typename t_type>
		t_type take() { return binary_parsing::take <t_type, byte_order>(range()); }

		template <data_member t_mem, template <data_member> typename t_base = field_>
		void read_field() { detail::read_field <byte_order, t_mem, t_base>(range(), target()); }

		template <typename t_adjust_fn, typename t_callback_fn>
		void adjust_range(t_adjust_fn &&adjust_fn, t_callback_fn &&callback_fn);

		template <typename t_iterable, typename t_fn>
		void for_(t_iterable &iterable, t_fn &&fn) { detail::for_ <byte_order>(*this, iterable, std::forward <t_fn>(fn)); }

	public:
		parser_(struct range &range_, t_target &target):
			m_range(&range_),
			m_target(&target)
		{
		}

		virtual void parse() = 0;
	};


	template <typename t_target, endian t_order>
	template <typename t_adjust_fn, typename t_callback_fn>
	void parser_ <t_target, t_order>::adjust_range(t_adjust_fn &&adjust_fn, t_callback_fn &&callback_fn)
	{
		auto new_range(*m_range);
		adjust_fn(new_range);

		auto *old_range(m_range);
		m_range = &new_range;

		callback_fn();

		m_range = old_range;
		m_range->it = new_range.it;
	}
}

#endif

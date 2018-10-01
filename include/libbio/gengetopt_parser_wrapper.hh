/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_GENGETOPT_PARSER_WRAPPER_HH
#define LIBBIO_GENGETOPT_PARSER_WRAPPER_HH


namespace libbio {
	
	// Wrap gengetopt_args_info mainly to call cmdline_parser_free when needed.
	class gengetopt_parser_wrapper
	{
	public:
		gengetopt_args_info	m_args{};
		
	public:
		gengetopt_parser_wrapper() = default;
		inline ~gengetopt_parser_wrapper() { cmdline_parser_free(&m_args); }
		gengetopt_parser_wrapper(gengetopt_parser_wrapper const &) = delete;
		gengetopt_parser_wrapper(gengetopt_parser_wrapper &&other) { using std::swap; swap(m_args, other.m_args); }
		gengetopt_parser_wrapper &operator=(gengetopt_parser_wrapper const &) & = delete;
		gengetopt_parser_wrapper &operator=(gengetopt_parser_wrapper &&other) & { using std::swap; swap(m_args, other.m_args); return *this; }
		
		inline void parse(int argc, char **argv);
		inline gengetopt_args_info &args() { return m_args; }
		inline gengetopt_args_info const &args() const { return m_args; }
	};
	
	
	void gengetopt_parser_wrapper::parse(int argc, char **argv)
	{
		if (0 != cmdline_parser(argc, argv, &m_args))
			exit(EXIT_FAILURE);
	}
}

#endif

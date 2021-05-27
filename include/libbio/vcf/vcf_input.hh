/*
 * Copyright (c) 2017-2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_INPUT_HH
#define LIBBIO_VCF_INPUT_HH

#include <libbio/file_handling.hh>
#include <libbio/mmap_handle.hh>


namespace libbio::vcf {
	
	class reader;
	
	
	class input_base
	{
		friend class reader;
		
	protected:
		std::size_t					m_first_variant_lineno{};
	
	public:
		virtual ~input_base() {}
		
		std::size_t first_variant_lineno() const { return m_first_variant_lineno; }
		std::size_t last_header_lineno() const { return m_first_variant_lineno - 1; }
		virtual char const *path() const { return "(unknown)"; }
		
	protected:
		virtual void reader_will_take_input() {}
		virtual char const *buffer_start() const = 0;
		virtual void fill_buffer(reader &vcf_reader) = 0;
		void set_first_variant_lineno(std::size_t lineno) { m_first_variant_lineno = lineno; }
	};
	
	
	class empty_input : public input_base
	{
	protected:
		virtual char const *buffer_start() const override { return nullptr; }
		virtual void fill_buffer(reader &vcf_reader) override;
	};
	
	
	class seekable_input_base : public input_base
	{
		// TODO: implement seeking.
	};
	
	
	// fill_buffer, buffer_start and reader_will_take_input are implemented here.
	class stream_input_base
	{
	protected:
		std::string					m_buffer;
		std::size_t					m_len{};
		std::size_t					m_pos{};
		
	public:
		stream_input_base() = default;
		
		explicit stream_input_base(std::size_t const buffer_size):
			m_buffer(buffer_size, '\0')
		{
		}
		
		virtual ~stream_input_base() {}
		
		virtual std::istream &stream() = 0;
		virtual std::istream const &stream() const = 0;
		
	protected:
		char const *buffer_start() const { return m_buffer.data() + m_pos; }
		void fill_buffer(reader &vcf_reader);
		void reader_will_take_input();
	};
	
	
	template <typename t_stream, typename t_base>
	class stream_input_tpl : public t_base, public stream_input_base
	{
	public:
		typedef t_stream			stream_type;
		
	protected:
		stream_type					m_stream;
		
	public:
		stream_input_tpl() = default;
		
		explicit stream_input_tpl(std::size_t const buffer_size):
			stream_input_base(buffer_size)
		{
		}
		
		stream_type &stream() override { return m_stream; }
		stream_type const &stream() const override { return m_stream; }
		
	protected:
		virtual void reader_will_take_input() override;
		char const *buffer_start() const override { return stream_input_base::buffer_start(); }
		void fill_buffer(reader &vcf_reader) override { stream_input_base::fill_buffer(vcf_reader); }
	};
	
	
	template <typename t_stream>
	class stream_input final : public stream_input_tpl <t_stream, input_base>
	{
	public:
		using stream_input_tpl <t_stream, input_base>::stream_input_tpl;
	};
	
	
	template <typename t_stream>
	class seekable_stream_input final : public stream_input_tpl <t_stream, seekable_input_base>
	{
	public:
		using stream_input_tpl <t_stream, seekable_input_base>::stream_input_tpl;
	};
	
	
	class mmap_input final : public seekable_input_base
	{
	public:
		typedef mmap_handle <char> handle_type;
		
	protected:
		handle_type					m_handle{};
		
	public:
		handle_type &handle() { return m_handle; }
		handle_type const &handle() const { return m_handle; }
		char const *path() const override { return m_handle.path().data(); }
		
	protected:
		char const *buffer_start() const override { return m_handle.data(); }
		void fill_buffer(reader &vcf_reader) override;
	};
	
	
	template <typename t_stream, typename t_base>
	void stream_input_tpl <t_stream, t_base>::reader_will_take_input()
	{
		t_base::reader_will_take_input();
		stream_input_base::reader_will_take_input();
	}
}

#endif

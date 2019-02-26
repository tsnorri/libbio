/*
 * Copyright (c) 2017-2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_VCF_INPUT_HH
#define LIBBIO_VCF_INPUT_HH

#include <libbio/file_handling.hh>
#include <libbio/mmap_handle.hh>


namespace libbio {
	
	class vcf_reader;
	
	
	class vcf_input
	{
	protected:
		std::size_t					m_first_variant_offset{0};
		std::size_t					m_first_variant_lineno{0};
		
	public:
		vcf_input() = default;
		explicit vcf_input(vcf_input const &other) = default;
		
		virtual ~vcf_input() {};
		virtual bool getline(std::string_view &dst) = 0;
		virtual void store_first_variant_offset(std::size_t const lineno) { m_first_variant_lineno = lineno; }
		virtual void reset_to_first_variant_offset() {}
		virtual void fill_buffer(vcf_reader &reader) = 0;
		
		virtual char const *buffer_start() const { return nullptr; }
		virtual char const *first_variant_start() const { return nullptr; }
		virtual char const *buffer_end() const { return nullptr; }
		virtual void set_range_start_lineno(std::size_t const lineno) {}
		virtual void set_range_start_offset(std::size_t const offset) {}
		virtual void set_range_length(std::size_t) {}
		std::size_t last_header_lineno() const { return m_first_variant_lineno - 1; }
	};


	class vcf_stream_input_base : public vcf_input
	{
	protected:
		std::string					m_buffer;
		std::size_t					m_len{0};
		std::size_t					m_pos{0};
	
	public:
		vcf_stream_input_base():
			m_buffer(128, '\0')
		{
		}
		
		virtual ~vcf_stream_input_base() {}
	
		virtual bool getline(std::string_view &dst) override;
		virtual void store_first_variant_offset(std::size_t const lineno) override;
		virtual void reset_to_first_variant_offset() override;
		virtual void fill_buffer(vcf_reader &reader) override;
		
	protected:
		virtual void stream_reset() = 0;
		virtual bool stream_getline() = 0;
		virtual std::size_t stream_tellg() = 0;
		virtual void stream_read(char *data, std::size_t len) = 0;
		virtual std::size_t stream_gcount() const = 0;
		virtual bool stream_eof() const = 0;
	};
	
	
	template <typename t_stream>
	class vcf_stream_input final : public vcf_stream_input_base
	{
	protected:
		t_stream					m_stream;
		
	public:
		using vcf_stream_input_base::vcf_stream_input_base;
		
		t_stream &input_stream() { return m_stream; }
		
	protected:
		virtual void stream_reset() override
		{
			m_stream.clear();
			m_stream.seekg(m_first_variant_offset);
		}
		
		virtual bool stream_getline() override							{ return std::getline(m_stream, m_buffer).operator bool(); }
		virtual std::size_t stream_tellg() override						{ return m_stream.tellg(); }
		virtual std::size_t stream_gcount() const override				{ return m_stream.gcount(); }
		virtual bool stream_eof() const override						{ return m_stream.eof(); }

		virtual void stream_read(char *data, std::size_t len) override
		{
			try
			{
				m_stream.read(data, len);
			}
			catch (std::ios_base::failure const &exc)
			{
				if (!m_stream.eof())
					throw (exc);
			}
		}
	};


	class vcf_mmap_input final : public vcf_input
	{
	public:
		typedef mmap_handle <char> handle_type;
		
	protected:
		handle_type const	*m_handle{nullptr};
		std::size_t			m_pos{0};
		std::size_t			m_range_start_lineno{0};
		std::size_t			m_range_start_offset{0};
		std::size_t			m_range_length{0};
	
	public:
		vcf_mmap_input() = default;
		
		vcf_mmap_input(handle_type const &handle):
			m_handle(&handle)
		{
			reset_range();
		}
		
		handle_type const &handle() const { return *m_handle; }
		
		virtual bool getline(std::string_view &dst) override;
		virtual void store_first_variant_offset(std::size_t const lineno) override;
		virtual void fill_buffer(vcf_reader &reader) override;
		
		virtual char const *buffer_start() const override { return m_handle->data(); }
		virtual char const *first_variant_start() const override { return m_handle->data() + m_first_variant_offset; }
		virtual char const *buffer_end() const override { return m_handle->data() + m_handle->size(); }
		void reset_range();
		virtual void set_range_start_lineno(std::size_t const lineno) override { m_range_start_lineno = lineno; }
		virtual void set_range_start_offset(std::size_t const offset) override { m_range_start_offset = offset; }
		virtual void set_range_length(std::size_t const length) override { m_range_length = length; }
	};
}

#endif

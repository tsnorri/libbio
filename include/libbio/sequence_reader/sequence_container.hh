/*
 * Copyright (c) 2018-2023 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_SEQUENCE_READER_SEQUENCE_CONTAINER_HH
#define LIBBIO_SEQUENCE_READER_SEQUENCE_CONTAINER_HH

#include <cstdint>
#include <libbio/mmap_handle.hh>
#include <iostream>
#include <vector>


namespace libbio { namespace sequence_reader {
	
	typedef std::vector <std::span <std::uint8_t const>>	sequence_vector;
	
	
	class sequence_container
	{
	protected:
		std::string		m_path;
		
	public:
		virtual ~sequence_container() {}
		virtual void to_spans(sequence_vector &dst) = 0;
		std::string const &path() const { return m_path; }
		void set_path(std::string const &path) { m_path = path; }
		void set_path(char const *path) { m_path = std::string(path); }
		
	protected:
		template <typename t_sequences>
		void to_spans(t_sequences const &src, sequence_vector &dst)
		{
			auto const src_size(src.size());
			dst.clear();
			dst.reserve(src_size);
			for (auto const &seq : src)
			{
				auto const &data(seq.data());
				auto const size(seq.size());
				dst.emplace_back(reinterpret_cast <std::uint8_t const *>(data), size);
			}
		}
	};
	
	
	class vector_sequence_container final : public sequence_container
	{
		friend std::ostream &operator<<(std::ostream &, vector_sequence_container const &);

	public:
		typedef std::vector <std::vector <std::uint8_t>>	buffer_type;
		
	protected:
		buffer_type											m_sequences;
		
	public:
		buffer_type &sequences() { return m_sequences; }
		virtual void to_spans(sequence_vector &dst) override { sequence_container::to_spans(m_sequences, dst); }
	};
	
	
	class mmap_sequence_container final : public sequence_container
	{
		friend std::ostream &operator<<(std::ostream &, mmap_sequence_container const &);

	public:
		typedef libbio::mmap_handle <char> handle_type;

	protected:
		handle_type											m_handle;
		std::size_t											m_sequence_length;
		std::size_t											m_sequence_count;
		
	public:
		void open_file(char const *path) { m_handle.open(path); }
		virtual void to_spans(sequence_vector &dst) override;
	};
	
	
	class multiple_mmap_sequence_container final : public sequence_container
	{
		friend std::ostream &operator<<(std::ostream &, multiple_mmap_sequence_container const &);
		
	public:
		typedef libbio::mmap_handle <char> handle_type;
		
	protected:
		std::vector <handle_type>							m_handles;
		
	public:
		void open_file(std::string const &path) { auto &handle(m_handles.emplace_back()); handle.open(path); }
		virtual void to_spans(sequence_vector &dst) override { sequence_container::to_spans(m_handles, dst); }
	};
	
	
	std::ostream &operator<<(std::ostream &stream, vector_sequence_container const &);
	std::ostream &operator<<(std::ostream &stream, mmap_sequence_container const &);
	std::ostream &operator<<(std::ostream &stream, multiple_mmap_sequence_container const &);
}}

#endif

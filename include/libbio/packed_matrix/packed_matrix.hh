/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_PACKED_MATRIX_PACKED_MATRIX_HH
#define LIBBIO_PACKED_MATRIX_PACKED_MATRIX_HH

#include <libbio/matrix/utility.hh>
#include <libbio/packed_matrix/fwd.hh>
#include <libbio/packed_matrix/iterator.hh>
#include <libbio/packed_matrix/slice.hh>
#include <libbio/packed_matrix/utility.hh>
#include <libbio/packed_vector.hh>


namespace libbio {
	
	template <unsigned int t_bits, typename t_word>
	class packed_matrix
	{
	public:
		typedef packed_vector <t_bits, t_word>									vector_type;
		typedef typename vector_type::word_type									word_type;
		typedef typename vector_type::reference_proxy							reference_proxy;
		
		typedef word_type														value_type;
		typedef reference_proxy													reference;
		typedef word_type														const_reference;
		
		typedef typename vector_type::iterator									iterator;
		typedef typename vector_type::const_iterator							const_iterator;
		typedef typename vector_type::word_iterator								word_iterator;
		typedef typename vector_type::const_word_iterator						const_word_iterator;
		typedef detail::packed_matrix_iterator <vector_type>					matrix_iterator;
		typedef detail::packed_matrix_iterator <vector_type const>				const_matrix_iterator;
		typedef detail::packed_vector_word_iterator_proxy <vector_type>			word_iterator_proxy;
		typedef detail::packed_vector_word_iterator_proxy <vector_type const>	const_word_iterator_proxy;

		typedef detail::packed_matrix_slice <packed_matrix>						slice_type;
		typedef detail::packed_matrix_slice <packed_matrix const>				const_slice_type;
		
		enum {
			ELEMENT_BITS		= t_bits,
			ELEMENT_COUNT		= vector_type::ELEMENT_COUNT,
			ELEMENT_MASK		= vector_type::ELEMENT_MASK,
			WORD_BITS			= vector_type::WORD_BITS
		};
		
		friend matrix_iterator;
		friend const_matrix_iterator;

	protected:
		vector_type											m_data;
#ifndef LIBBIO_NDEBUG
		std::size_t											m_columns{};
#endif
		std::size_t											m_stride{1};
		
	public:
		packed_matrix() = default;
		
		template <bool t_dummy = true, std::enable_if_t <t_dummy && 0 != t_bits, int> = 0>
		packed_matrix(std::size_t const rows, std::size_t const columns):
			m_data(columns * rows),
#ifndef LIBBIO_NDEBUG
			m_columns(columns),
#endif
			m_stride(rows)
		{
			libbio_assert(m_stride);
		}
		
		packed_matrix(std::size_t const rows, std::size_t const columns, std::uint8_t const bits):
			m_data(columns * rows, bits),
#ifndef LIBBIO_NDEBUG
			m_columns(columns),
#endif
			m_stride(rows)
		{
			libbio_assert(m_stride);
		}
		
		inline std::size_t idx(std::size_t const y, std::size_t const x) const
		{
			/* Column major order. */
			libbio_assert(y < m_stride);
			libbio_assert(x < m_columns);
			libbio_assert(x < m_data.size() / m_stride);
			std::size_t const retval(x * m_stride + y);
			libbio_assert(retval < m_data.size());
			return retval;
		}
		
		// Primitives.
		word_type load(std::size_t const y, std::size_t const x, std::memory_order order = std::memory_order_seq_cst) const { return m_data.load(idx(y, x), order); }
		word_type fetch_or(std::size_t const y, std::size_t const x, std::memory_order order = std::memory_order_seq_cst) const { return m_data.fetch_or(idx(y, x), order); }
		
		word_type operator()(std::size_t const y, std::size_t const x, std::memory_order order = std::memory_order_seq_cst) const { return load(y, x, order); };
		reference_proxy operator()(std::size_t const y, std::size_t const x) { return m_data(idx(y, x)); }
		
		std::size_t size() const { return m_data.size(); }
		std::size_t available_size() const { return m_data.available_size(); }
		std::size_t word_size() const { return m_data.word_size(); }
		void set_size(std::size_t new_size) { m_data.set_size(new_size); }
		std::size_t const number_of_columns() const { return m_data.size() / m_stride; }
		std::size_t const number_of_rows() const { return m_stride; }
		std::size_t const stride() const { return m_stride; }
		void set_stride(std::size_t const stride) { m_stride = stride; }
		
		vector_type const &values() const { return m_data; }
		
		constexpr std::size_t word_bits() const { return m_data.word_bits(); }
		
		// The following are constexpr iff t_bits != 0.
		template <bool t_dummy = true, std::enable_if_t <t_dummy && 0 != t_bits, int> = 0>
		constexpr std::size_t element_bits() const { return m_data.element_bits(); }
		
		template <bool t_dummy = true, std::enable_if_t <t_dummy && 0 != t_bits, int> = 0>
		constexpr std::size_t element_count_in_word() const { return m_data.element_count_in_word(); }
		
		template <bool t_dummy = true, std::enable_if_t <t_dummy && 0 != t_bits, int> = 0>
		constexpr word_type element_mask() const { return m_data.element_mask(); }
		
		template <bool t_dummy = true, std::enable_if_t <t_dummy && 0 == t_bits, int> = 0>
		std::size_t element_bits() const { return m_data.element_bits(); }
		
		template <bool t_dummy = true, std::enable_if_t <t_dummy && 0 == t_bits, int> = 0>
		std::size_t element_count_in_word() const { return m_data.element_count_in_word(); }
		
		template <bool t_dummy = true, std::enable_if_t <t_dummy && 0 == t_bits, int> = 0>
		word_type element_mask() const { return m_data.element_mask(); }
		
		// Iterators to packed values.
		iterator begin() { return m_data.begin(); }
		iterator end() { return m_data.end(); }
		const_iterator begin() const { return m_data.begin(); }
		const_iterator end() const { return m_data.end(); }
		const_iterator cbegin() const { return m_data.cbegin(); }
		const_iterator cend() const { return m_data.cend(); }
		
		// Iterators to whole words.
		word_iterator word_begin() { return m_data.word_begin(); }
		word_iterator word_end() { return m_data.word_end(); }
		const_word_iterator word_begin() const { return m_data.word_begin(); }
		const_word_iterator word_end() const { return m_data.word_end(); }
		const_word_iterator word_cbegin() const { return m_data.word_cbegin(); }
		const_word_iterator word_cend() const { return m_data.word_cend(); }
		word_iterator_proxy word_range() { return word_iterator_proxy(m_data); }
		const_word_iterator_proxy word_range() const { return const_word_iterator_proxy(m_data); }
		const_word_iterator_proxy const_word_range() const { return const_word_iterator_proxy(m_data); }

		// Slices.
		slice_type row(std::size_t const row, std::size_t const first = 0)												{ return matrices::row(*this, row, first, this->number_of_columns()); }
		slice_type column(std::size_t const column, std::size_t const first = 0)										{ return matrices::column(*this, column, first, this->number_of_rows()); }
		slice_type row(std::size_t const row, std::size_t const first, std::size_t const limit)							{ return matrices::row(*this, row, first, limit); }
		slice_type column(std::size_t const column, std::size_t const first, std::size_t const limit)					{ return matrices::column(*this, column, first, limit); }
		const_slice_type row(std::size_t const row, std::size_t const first = 0) const									{ return matrices::const_row(*this, row, first, this->number_of_columns()); }
		const_slice_type column(std::size_t const column, std::size_t const first = 0) const							{ return matrices::const_column(*this, column, first, this->number_of_rows()); }
		const_slice_type row(std::size_t const row, std::size_t const first, std::size_t const limit) const				{ return matrices::const_row(*this, row, first, limit); }
		const_slice_type column(std::size_t const column, std::size_t const first, std::size_t const limit) const		{ return matrices::const_column(*this, column, first, limit); }
		const_slice_type const_row(std::size_t const row, std::size_t const first = 0) const							{ return matrices::const_row(*this, row, first, this->number_of_columns()); }
		const_slice_type const_column(std::size_t const column, std::size_t const first = 0) const						{ return matrices::const_column(*this, column, first, this->number_of_rows()); }
		const_slice_type const_row(std::size_t const row, std::size_t const first, std::size_t const limit) const		{ return matrices::const_row(*this, row, first, limit); }
		const_slice_type const_column(std::size_t const column, std::size_t const first, std::size_t const limit) const	{ return matrices::const_column(*this, column, first, limit); }
	};
	
	
	template <unsigned int t_bits, typename t_word>
	std::ostream &operator<<(std::ostream &os, packed_matrix <t_bits, t_word> const &matrix)
	{
		auto const row_count(matrix.number_of_rows());
		for (std::size_t i(0); i < row_count; ++i)
		{
			auto const &row(matrix.row(i));
			ranges::copy(row | ranges::view::transform([](auto val){ return +val; }), ranges::make_ostream_joiner(os, "\t"));
			os << '\n';
		}
		return os;
	}
}

#endif

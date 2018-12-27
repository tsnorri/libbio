/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_MATRIX_MATRIX_HH
#define LIBBIO_MATRIX_MATRIX_HH

#include <libbio/assert.hh>
#include <libbio/matrix/iterator.hh>
#include <libbio/matrix/slice.hh>
#include <libbio/matrix/utility.hh>
#include <type_traits>
#include <vector>


namespace libbio {
	
	// Wrap an std::vector so that parts of it that correspond to matrix rows and columns may be used with
	// std and Boost Range algorithms. Said parts may be obtained with row() and column().
	template <typename t_value>
	class matrix
	{
		friend class detail::matrix_slice <matrix>;
		
	public:
		typedef t_value											value_type;
		
	protected:
		typedef std::vector <value_type>						vector_type;
		
	public:
		typedef typename vector_type::reference					reference;
		typedef typename vector_type::const_reference			const_reference;
		
		typedef typename vector_type::iterator					iterator;
		typedef typename vector_type::const_iterator			const_iterator;
		
		typedef detail::matrix_iterator <iterator>				matrix_iterator;
		typedef detail::matrix_iterator <const_iterator>		const_matrix_iterator;
		
		typedef detail::matrix_slice <matrix>					slice_type;
		typedef detail::matrix_slice <matrix const>				const_slice_type;
		
	protected:
		std::vector <value_type>	m_data;
#ifndef LIBBIO_NDEBUG
		std::size_t					m_columns{};
#endif
		std::size_t					m_stride{1};
		
	protected:
		void resize_if_needed(std::size_t const rows, std::size_t const cols);
		
	public:
		matrix() = default;
		matrix(std::size_t const rows, std::size_t const columns):
			m_data(columns * rows, 0),
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
		
		value_type &operator()(std::size_t const y, std::size_t const x) { return m_data[idx(y, x)]; }
		value_type const &operator()(std::size_t const y, std::size_t const x) const { return m_data[idx(y, x)]; }
		
		std::size_t const size() const { return m_data.size(); }
		std::size_t const number_of_columns() const { return m_data.size() / m_stride; }
		std::size_t const number_of_rows() const { return m_stride; }
		std::size_t const stride() const { return m_stride; }
		void resize(std::size_t const rows, std::size_t const cols) { resize_if_needed(rows, cols); }
		void resize(std::size_t const size) { m_data.resize(size); }
		void set_stride(std::size_t const stride) { libbio_always_assert(0 == m_data.size() % m_stride); m_stride = stride; }
		
		template <typename t_fn>
		void apply(t_fn &&fn);
		
		void swap(matrix &rhs);
		
		// Iterators.
		iterator begin() { return m_data.begin(); }
		iterator end() { return m_data.end(); }
		const_iterator begin() const { return m_data.cbegin(); }
		const_iterator end() const { return m_data.cend(); }
		const_iterator cbegin() const { return m_data.cbegin(); }
		const_iterator cend() const { return m_data.cend(); }
		
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
	
	
	template <typename t_value>
	void swap(matrix <t_value> &lhs, matrix <t_value> &rhs)
	{
		lhs.swap(rhs);
	}
	
	
	template <typename t_value>
	std::ostream &operator<<(std::ostream &os, matrix <t_value> const &matrix)
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
	
	
	template <typename t_value>
	void matrix <t_value>::resize_if_needed(std::size_t const rows, std::size_t const columns)
	{
		if (number_of_rows() < rows || number_of_columns() < columns)
		{
			if (size() < rows * columns)
				resize(rows * columns);

			set_stride(rows);
#ifndef LIBBIO_NDEBUG
			m_columns = columns;
#endif
		}
	}
	
	
	template <typename t_value>
	void matrix <t_value>::swap(matrix <t_value> &rhs)
	{
		using std::swap;
		swap(m_data, rhs.m_data);
		swap(m_stride, rhs.m_stride);
#ifndef LIBBIO_NDEBUG
		swap(m_columns, rhs.m_columns);
#endif
	}
	
	
	template <typename t_value>
	template <typename t_fn>
	void matrix <t_value>::apply(t_fn &&value_type)
	{
		for (auto &val : m_data)
			val = t_fn(val);
	}
}

#endif

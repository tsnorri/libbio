/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_MATRIX_HH
#define LIBBIO_MATRIX_HH

#include <boost/iterator/iterator_facade.hpp>
#include <boost/range.hpp>
#include <iterator>
#include <type_traits>
#include <valarray>
#include <vector>


namespace libbio { namespace detail {
	
	template <typename t_iterator>
	class matrix_iterator : public boost::iterator_facade <
		matrix_iterator <t_iterator>,
		std::remove_reference_t <typename std::iterator_traits <t_iterator>::reference>,
		boost::random_access_traversal_tag
	>
	{
		friend class boost::iterator_core_access;
		
	private:
		typedef typename std::iterator_traits <t_iterator>::reference		iterator_reference_type;
		
	public:
		typedef typename std::remove_reference_t <iterator_reference_type> 	value_type;
		
	private:
		t_iterator			m_it{};
		std::ptrdiff_t		m_stride{1}; // Need signed type for division in distance_to().

	public:
		matrix_iterator() = default;

		matrix_iterator(t_iterator it, std::size_t stride):
			m_it(it),
			m_stride(stride)
		{
		}
		
		template <
			typename t_other_iterator,
			std::enable_if_t <std::is_convertible_v <t_other_iterator *, t_iterator *>>
		> matrix_iterator(matrix_iterator <t_other_iterator> const &other):
			m_it(other.m_it),
			m_stride(other.m_stride)
		{
		}
		
	private:
		value_type &dereference() const { return *m_it; }
		bool equal(matrix_iterator const &other) const { return m_it == other.m_it && m_stride == other.m_stride; }
		void increment() { advance(1); }
		void advance(std::ptrdiff_t const diff) { std::advance(m_it, m_stride * diff); }
		
		std::ptrdiff_t distance_to(matrix_iterator const &other) const
		{
			auto const dist(std::distance(m_it, other.m_it));
			auto const retval(dist / m_stride);
			return retval;
		}
	};
	
	
	template <typename t_iterator>
	std::ostream &operator<<(std::ostream &stream, matrix_iterator <t_iterator> const &it) { stream << "(matrix iterator)"; return stream; }
	
	
	template <typename t_matrix>
	class matrix_slice
	{
	public:
		typedef t_matrix										matrix_type;
		typedef typename matrix_type::size_type					size_type;
		typedef typename matrix_type::value_type				value_type;
		typedef typename matrix_type::iterator					matrix_iterator;
		typedef typename matrix_type::const_iterator			const_matrix_iterator;
		typedef boost::iterator_range <matrix_iterator>			matrix_iterator_range;
		typedef boost::iterator_range <const_matrix_iterator>	const_matrix_iterator_range;
		
	protected:
		t_matrix	*m_matrix{nullptr};
		std::slice	m_slice{};
		
	public:
		matrix_slice() = default;
		matrix_slice(t_matrix &matrix, std::slice const &slice):
			m_matrix(&matrix),
			m_slice(slice)
		{
		}
		
		size_type size() const { return m_slice.size(); }
		
		value_type &operator[](size_type const idx) { return *(begin() + idx); }
		value_type const &operator[](size_type const idx) const { return *(begin() + idx); }
		
		matrix_iterator begin() { return matrix_iterator(m_matrix->begin() + m_slice.start(), m_slice.stride()); }
		matrix_iterator end() { return matrix_iterator(m_matrix->begin() + m_slice.start() + m_slice.size() * m_slice.stride(), m_slice.stride()); }
		matrix_iterator range() { return boost::make_iterator_range(begin(), end()); }
		const_matrix_iterator begin() const { return cbegin(); }
		const_matrix_iterator end() const { return cend(); }
		const_matrix_iterator_range range() const { return const_range(); }
		const_matrix_iterator cbegin() const { return const_matrix_iterator(m_matrix->cbegin() + m_slice.start(), m_slice.stride()); }
		const_matrix_iterator cend() const { return const_matrix_iterator(m_matrix->cbegin() + m_slice.start() + m_slice.size() * m_slice.stride(), m_slice.stride()); }
		const_matrix_iterator_range const_range() const { return boost::make_iterator_range(cbegin(), cend()); }
	};
	
	
	template <typename t_matrix>
	class matrix_element_reference
	{
	protected:
		typedef typename t_matrix::size_type size_type;
		
	protected:
		t_matrix	*m_matrix{nullptr};
		size_type	m_x{0};
		size_type	m_y{0};
		
	public:
		matrix_element_reference() = delete;
		matrix_element_reference(t_matrix &matrix, size_type const y, size_type const x):
			m_matrix(&matrix),
			m_x(x),
			m_y(y)
		{
		}
		
		operator std::uint8_t() const { return m_matrix->fetch(m_y, m_x); }
		matrix_element_reference &operator=(std::uint64_t const val) { m_matrix->fetch_or(m_y, m_x, val); return *this; }
		std::uint64_t fetch_or(std::uint64_t const val) { return m_matrix->fetch_or(m_y, m_x, val); }
	};
	
	
	template <typename t_matrix> typename t_matrix::slice_type row(
		t_matrix &matrix,
		typename t_matrix::size_type const row, 
		typename t_matrix::size_type const first, 
		typename t_matrix::size_type const limit
	);
	
	template <typename t_matrix> typename t_matrix::slice_type column(
		t_matrix &matrix,
		typename t_matrix::size_type const column,
		typename t_matrix::size_type const first,
		typename t_matrix::size_type const limit
	);
	
	template <typename t_matrix> typename t_matrix::const_slice_type const_row(
		t_matrix const &matrix,
		typename t_matrix::size_type const row,
		typename t_matrix::size_type const first,
		typename t_matrix::size_type const limit
	);
	
	template <typename t_matrix> typename t_matrix::const_slice_type const_column(
		t_matrix const &matrix,
		typename t_matrix::size_type const column,
		typename t_matrix::size_type const first,
		typename t_matrix::size_type const limit
	);
}}


namespace libbio {
	
	// Wrap an std::vector so that parts of it that correspond to matrix rows and columns may be used with
	// std and Boost Range algorithms. Said parts may be obtained with row() and column().
	template <typename t_value>
	class matrix
	{
		friend class detail::matrix_slice <matrix>;
		friend class detail::matrix_element_reference <matrix>;
		
	public:
		typedef t_value											value_type;
		typedef std::vector <value_type>						vector_type;
		typedef typename vector_type::size_type					size_type;
		typedef typename vector_type::iterator					vector_iterator;
		typedef typename vector_type::const_iterator			const_vector_iterator;
		typedef detail::matrix_iterator <vector_iterator>		iterator;
		typedef detail::matrix_iterator <const_vector_iterator>	const_iterator;
		typedef detail::matrix_slice <matrix>					slice_type;
		typedef detail::matrix_slice <matrix const>				const_slice_type;
		
	protected:
		vector_type					m_values;
#ifndef NDEBUG
		size_type					m_columns{0};
#endif
		size_type					m_stride{1};
		
	protected:
		void resize_if_needed(size_type const rows, size_type const cols);
		
	public:
		matrix() = default;
		matrix(size_type const rows, size_type const columns, value_type const val):
			m_values(columns * rows, val),
#ifndef NDEBUG
			m_columns(columns),
#endif
			m_stride(rows)
		{
			assert(m_stride);
		}
		
		matrix(size_type const rows, size_type const columns):
			m_values(columns * rows),
#ifndef NDEBUG
			m_columns(columns),
#endif
			m_stride(rows)
		{
			assert(m_stride);
		}
		
		inline size_type idx(size_type const y, size_type const x) const
		{
			/* Column major order. */
			assert(y < m_stride);
			assert(x < m_columns);
			assert(x < m_values.size() / m_stride);
			size_type const retval(x * m_stride + y);
			assert(retval < m_values.size());
			return retval;
		}
		
		size_type const size() const { return m_values.size(); }
		size_type const column_size() const { return m_values.size() / m_stride; }
		size_type const row_size() const { return m_stride; }
		size_type const stride() const { return m_stride; }
		void resize(size_type const rows, size_type const cols) { resize_if_needed(rows, cols); }
		void resize(size_type const size) { m_values.resize(size); }
		void set_stride(size_type const stride) { always_assert(0 == m_values.size() % m_stride); m_stride = stride; }
		value_type &operator()(size_type const y, size_type const x) { return m_values[idx(y, x)]; }
		value_type const &operator()(size_type const y, size_type const x) const { return m_values[idx(y, x)]; }
		slice_type row(size_type const row, size_type const first = 0)												{ return detail::row <matrix>			(*this, row, first, this->column_size()); }
		slice_type column(size_type const column, size_type const first = 0)										{ return detail::column <matrix>		(*this, column, first, this->row_size()); }
		slice_type row(size_type const row, size_type const first, size_type const limit)							{ return detail::row <matrix>			(*this, row, first, limit); }
		slice_type column(size_type const column, size_type const first, size_type const limit)						{ return detail::column <matrix>		(*this, column, first, limit); }
		const_slice_type row(size_type const row, size_type const first = 0) const									{ return detail::const_row <matrix>		(*this, row, first, this->column_size()); }
		const_slice_type column(size_type const column, size_type const first = 0) const							{ return detail::const_column <matrix>	(*this, column, first, this->row_size()); }
		const_slice_type row(size_type const row, size_type const first, size_type const limit) const				{ return detail::const_row <matrix>		(*this, row, first, limit); }
		const_slice_type column(size_type const column, size_type const first, size_type const limit) const			{ return detail::const_column <matrix>	(*this, column, first, limit); }
		const_slice_type const_row(size_type const row, size_type const first = 0) const							{ return detail::const_row <matrix>		(*this, row, first, this->column_size()); }
		const_slice_type const_column(size_type const column, size_type const first = 0) const						{ return detail::const_column <matrix>	(*this, column, first, this->row_size()); }
		const_slice_type const_row(size_type const row, size_type const first, size_type const limit) const			{ return detail::const_row <matrix>		(*this, row, first, limit); }
		const_slice_type const_column(size_type const column, size_type const first, size_type const limit) const	{ return detail::const_column <matrix>	(*this, column, first, limit); }
		vector_iterator begin() { return m_values.begin(); }
		vector_iterator end() { return m_values.end(); }
		const_vector_iterator begin() const { return m_values.cbegin(); }
		const_vector_iterator end() const { return m_values.cend(); }
		const_vector_iterator cbegin() const { return m_values.cbegin(); }
		const_vector_iterator cend() const { return m_values.cend(); }
		
		template <typename t_fn>
		void apply(t_fn &&fn);
		
		void swap(matrix &rhs);
	};
	
	
	// Store elements of 2^k bits where k = 0, 1, 2.
	// Specialize only operator() for now.
	template <std::uint8_t t_bits>
	class compressed_atomic_matrix : public matrix <std::atomic_uint64_t>
	{
		static_assert(1 == t_bits || 2 == t_bits || 4 == t_bits);
		
		friend class detail::matrix_slice <compressed_atomic_matrix>;
		friend class detail::matrix_element_reference <compressed_atomic_matrix>;
		
	protected:
		typedef matrix <std::atomic_uint64_t>								superclass;
		typedef detail::matrix_element_reference <compressed_atomic_matrix>	reference_proxy;
		
	public:
		enum {
			SUBELEMENT_COUNT	= 64 / t_bits,
			SUBELEMENT_MASK		= (std::numeric_limits <uint8_t>::max() >> (8 - t_bits))
		};
		
		typedef detail::matrix_slice <compressed_atomic_matrix>				slice_type;
		typedef detail::matrix_slice <compressed_atomic_matrix const>		const_slice_type;
		
	protected:
		std::uint64_t fetch(size_type const y, size_type const x) const;
		std::uint64_t fetch_or(size_type const row, size_type const col, std::uint64_t const val);
		void assign(size_type const y, size_type const x, std::uint64_t const val);
		
	public:
		using matrix::matrix;
		
		std::uint64_t value_at(size_type const y, size_type const x) const { return fetch(y, x); };
		std::uint64_t operator()(size_type const y, size_type const x) const { return value_at(y, x); };
		reference_proxy operator()(size_type const y, size_type const x) { return reference_proxy(*this, y, x); }
		
		// FIXME: rename these to block_* or something similar.
		slice_type row(size_type const row, size_type const first = 0)												{ return detail::row <compressed_atomic_matrix>				(*this, row, first, this->column_size()); }
		slice_type column(size_type const column, size_type const first = 0)										{ return detail::column <compressed_atomic_matrix>			(*this, column, first, this->row_size()); }
		slice_type row(size_type const row, size_type const first, size_type const limit)							{ return detail::row <compressed_atomic_matrix>				(*this, row, first, limit); }
		slice_type column(size_type const column, size_type const first, size_type const limit)						{ return detail::column <compressed_atomic_matrix>			(*this, column, first, limit); }
		const_slice_type row(size_type const row, size_type const first = 0) const									{ return detail::const_row <compressed_atomic_matrix>		(*this, row, first, this->column_size()); }
		const_slice_type column(size_type const column, size_type const first = 0) const							{ return detail::const_column <compressed_atomic_matrix>	(*this, column, first, this->row_size()); }
		const_slice_type row(size_type const row, size_type const first, size_type const limit) const				{ return detail::const_row <compressed_atomic_matrix>		(*this, row, first, limit); }
		const_slice_type column(size_type const column, size_type const first, size_type const limit) const			{ return detail::const_column <compressed_atomic_matrix>	(*this, column, first, limit); }
		const_slice_type const_row(size_type const row, size_type const first = 0) const							{ return detail::const_row <compressed_atomic_matrix>		(*this, row, first, this->column_size()); }
		const_slice_type const_column(size_type const column, size_type const first = 0) const						{ return detail::const_column <compressed_atomic_matrix>	(*this, column, first, this->row_size()); }
		const_slice_type const_row(size_type const row, size_type const first, size_type const limit) const			{ return detail::const_row <compressed_atomic_matrix>		(*this, row, first, limit); }
		const_slice_type const_column(size_type const column, size_type const first, size_type const limit) const	{ return detail::const_column <compressed_atomic_matrix>	(*this, column, first, limit); }
	};
	
	
	template <typename t_matrix>
	void initialize_atomic_matrix(
		t_matrix &matrix,
		typename t_matrix::size_type const rows,
		typename t_matrix::size_type const cols
	)
	{
		using std::swap;
		
		auto const row_size(matrix.row_size());
		auto const column_size(matrix.column_size());
		if (row_size < rows || column_size < cols)
		{
			if (matrix.size() < rows * cols)
			{
				t_matrix temp(rows, cols);
				swap(matrix, temp);
			}
			else
			{
				matrix.set_stride(rows);
			}
		}
	}
	
	
	template <typename t_value>
	void swap(matrix <t_value> &lhs, matrix <t_value> &rhs)
	{
		lhs.swap(rhs);
	}
	
	
	template <typename t_dst_value, typename t_src_value>
	void transpose_column_to_row(
		detail::matrix_slice <matrix <t_dst_value>> &&dst,
		detail::matrix_slice <matrix <std::atomic <t_src_value>>> const &src
	)
	{
		transpose_column_to_row(dst, src);
	}
	
	
	template <typename t_dst_value, typename t_src_value>
	void transpose_column_to_row(
		detail::matrix_slice <matrix <t_dst_value>> &&dst,
		detail::matrix_slice <matrix <t_src_value>> const &src
	)
	{
		transpose_column_to_row(dst, src);
	}
	
	
	template <std::uint8_t t_bits, typename t_value>
	void transpose_column_to_row(
		detail::matrix_slice <compressed_atomic_matrix <t_bits>> &&dst,
		detail::matrix_slice <matrix <t_value>> const &src
	)
	{
		transpose_column_to_row(dst, src);
	}
	
	
	template <std::uint8_t t_bits>
	void transpose_column_to_row(
		detail::matrix_slice <compressed_atomic_matrix <t_bits>> &&dst,
		detail::matrix_slice <compressed_atomic_matrix <t_bits>> const &src
	)
	{
		transpose_column_to_row(dst, src);
	}
	
	
	// std::atomic has a deleted copy assignment operator in the case where the source is also an atomic.
	// We would always like to have load followed by a store, so specialize transpose_column_to_row.
	template <typename t_dst_value, typename t_src_value>
	void transpose_column_to_row(
		detail::matrix_slice <matrix <t_dst_value>> &dst,
		detail::matrix_slice <matrix <std::atomic <t_src_value>>> const &src
	)
	{
		auto const length(dst.size());
		assert(src.size() <= length);
		std::transform(src.cbegin(), src.cend(), dst.begin(), [](auto &atomic){ return atomic.load(); });
	}
	
	
	template <typename t_dst_value, typename t_src_value>
	void transpose_column_to_row(
		detail::matrix_slice <matrix <t_dst_value>> &dst,
		detail::matrix_slice <matrix <t_src_value>> const &src
	)
	{
		auto const length(dst.size());
		assert(src.size() <= length);
		std::copy(src.cbegin(), src.cend(), dst.begin());
	}
	
	
	template <std::uint8_t t_bits, typename t_value>
	void transpose_column_to_row(
		detail::matrix_slice <compressed_atomic_matrix <t_bits>> &dst,
		detail::matrix_slice <matrix <t_value>> const &src
	)
	{
		typedef typename decltype(dst)::size_type size_type;
		
		uint64_t mask(1);
		mask <<= t_bits;
		--mask;
		
		auto const length(dst.size());
		assert(src.size() * (64 / t_bits) <= length);
		auto src_it(src.cbegin());
		for (size_type i(0); i < length; ++i)
		{
			auto const val(*src_it++);
			assert(0 == val >> t_bits);
			dst[i].fetch_or(val);
		}
	}
	
	
	template <std::uint8_t t_bits>
	void transpose_column_to_row(
		detail::matrix_slice <compressed_atomic_matrix <t_bits>> &dst,
		detail::matrix_slice <compressed_atomic_matrix <t_bits>> const &src
	)
	{
		typedef typename decltype(dst)::size_type size_type;

		uint64_t mask(1);
		mask <<= t_bits;
		--mask;
		
		auto const length(dst.size());
		assert(src.size() * (64 / t_bits) <= length);
		auto src_it(src.cbegin());
		uint64_t val(0);
		for (size_type i(0); i < length; ++i)
		{
			if (0 == i % (64 / t_bits))
				val = *src_it++;
			
			dst[i].fetch_or(val & mask);
			val >>= t_bits;
		}
	}
	
	
	template <typename t_slice>
	void fill_column_with_2_bit_pattern(t_slice &column, std::uint64_t pattern)
	{
		// Create a pattern for filling one word at a time.
		pattern |= pattern << 2;
		pattern |= pattern << 4;
		pattern |= pattern << 8;
		pattern |= pattern << 16;
		pattern |= pattern << 32;
		
		std::fill(column.begin(), column.end(), pattern);
	}
	
	
	template <typename t_value>
	void matrix <t_value>::resize_if_needed(size_type const rows, size_type const columns)
	{
		if (row_size() < rows || column_size() < columns)
		{
			if (size() < rows * columns)
				resize(rows * columns);

			set_stride(rows);
#ifndef NDEBUG
			m_columns = columns;
#endif
		}
	}
	
	
	template <typename t_value>
	void matrix <t_value>::swap(matrix <t_value> &rhs)
	{
		using std::swap;
		swap(m_values, rhs.m_values);
		swap(m_stride, rhs.m_stride);
#ifndef NDEBUG
		swap(m_columns, rhs.m_columns);
#endif
	}
	
	
	template <typename t_value>
	template <typename t_fn>
	void matrix <t_value>::apply(t_fn &&value_type)
	{
		for (auto &val : m_values)
			val = t_fn(val);
	}
	
	
	template <std::uint8_t t_bits>
	std::uint64_t compressed_atomic_matrix <t_bits>::fetch(size_type const row, size_type const col) const
	{
		auto const shift_amt(row % SUBELEMENT_COUNT * t_bits);
		auto const y(row / SUBELEMENT_COUNT);
		uint64_t val(superclass::operator()(y, col)); // Atomic.
		val >>= shift_amt;
		val &= SUBELEMENT_MASK;
		return val;
	}
	
	
	template <std::uint8_t t_bits>
	std::uint64_t compressed_atomic_matrix <t_bits>::fetch_or(size_type const row, size_type const col, std::uint64_t val)
	{
		assert(val <= SUBELEMENT_MASK);
		auto const shift_amt(row % SUBELEMENT_COUNT * t_bits);
		auto const y(row / SUBELEMENT_COUNT);
		val <<= shift_amt;
		auto const res(superclass::operator()(y, col).fetch_or(val)); // Atomic.
		return ((res >> shift_amt) & SUBELEMENT_MASK);
	}
}


namespace libbio { namespace detail {
	
	template <typename t_matrix>
	typename t_matrix::slice_type row(
		t_matrix &matrix,
		typename t_matrix::size_type const row,
		typename t_matrix::size_type const first,
		typename t_matrix::size_type const limit
	)
	{
		std::slice const slice(matrix.idx(row, first), limit - first, matrix.stride());
		return typename t_matrix::slice_type(matrix, slice);
	}
	
	
	template <typename t_matrix>
	typename t_matrix::slice_type column(
		t_matrix &matrix,
		typename t_matrix::size_type const column,
		typename t_matrix::size_type const first,
		typename t_matrix::size_type const limit
	)
	{
		std::slice const slice(matrix.idx(first, column), limit - first, 1);
		return typename t_matrix::slice_type(matrix, slice);
	}
	
	
	template <typename t_matrix>
	typename t_matrix::const_slice_type const_row(
		t_matrix const &matrix,
		typename t_matrix::size_type const row,
		typename t_matrix::size_type const first,
		typename t_matrix::size_type const limit
	)
	{
		std::slice const slice(matrix.idx(row, first), limit - first, matrix.stride());
		return typename t_matrix::const_slice_type(matrix, slice);
	}
	
	
	template <typename t_matrix>
	typename t_matrix::const_slice_type const_column(
		t_matrix const &matrix,
		typename t_matrix::size_type const column,
		typename t_matrix::size_type const first,
		typename t_matrix::size_type const limit
	)
	{
		std::slice const slice(matrix.idx(first, column), limit - first, 1);
		return typename t_matrix::const_slice_type(matrix, slice);
	}
}}

#endif

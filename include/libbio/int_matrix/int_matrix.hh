/*
 * Copyright (c) 2018–2019 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_INT_MATRIX_INT_MATRIX_HH
#define LIBBIO_INT_MATRIX_INT_MATRIX_HH

#include <libbio/int_matrix/iterator.hh>
#include <libbio/int_matrix/slice.hh>
#include <libbio/int_vector.hh>
#include <libbio/matrix/indexing.hh>
#include <libbio/matrix/utility.hh>


namespace libbio {
	
	template <unsigned int t_bits, typename t_word, template <typename, unsigned int, typename> typename t_trait>
	class int_matrix_tpl;
}


namespace libbio { namespace detail {
	
	// Traits / mixins.
	
	template <typename t_matrix, unsigned int t_bits, typename t_word>
	struct int_matrix_trait
	{
		typedef t_word									word_type;
		typedef int_vector <t_bits, t_word>				vector_type;
		typedef typename vector_type::reference_proxy	reference_proxy;
		
		word_type operator()(std::size_t const y, std::size_t const x) const { auto &self(as_matrix()); return self.m_data[self.idx(y, x)]; }
		reference_proxy operator()(std::size_t const y, std::size_t const x) { auto &self(as_matrix()); return self.m_data[self.idx(y, x)]; }
		
	protected:
		t_matrix &as_matrix() { return static_cast <t_matrix &>(*this); }
		t_matrix const &as_matrix() const { return static_cast <t_matrix const &>(*this); }
	};
	
	
	template <typename t_matrix, unsigned int t_bits, typename t_word>
	struct atomic_int_matrix_trait
	{
		typedef t_word									word_type;
		typedef atomic_int_vector <t_bits, t_word>		vector_type;
		typedef typename vector_type::reference_proxy	reference_proxy;
		
		inline word_type load(std::size_t const y, std::size_t const x, std::memory_order order = std::memory_order_seq_cst) const;
		inline word_type fetch_or(std::size_t const y, std::size_t const x, std::memory_order order = std::memory_order_seq_cst) const;
		word_type operator()(std::size_t const y, std::size_t const x, std::memory_order order = std::memory_order_seq_cst) const { return load(y, x, order); };
		inline reference_proxy operator()(std::size_t const y, std::size_t const x);
		
	protected:
		t_matrix &as_matrix() { return static_cast <t_matrix &>(*this); }
		t_matrix const &as_matrix() const { return static_cast <t_matrix const &>(*this); }
	};
	
	
	// Trait / mixin for accessing element count and mask.
	template <typename t_matrix>
	struct int_matrix_width_base
	{
		t_matrix &as_matrix() { return static_cast <t_matrix &>(*this); }
		t_matrix const &as_matrix() const { return static_cast <t_matrix const &>(*this); }
	};
	
	
	template <typename t_matrix, unsigned int t_bits, typename t_word>
	struct int_matrix_width : public int_matrix_width_base <t_matrix>
	{
		constexpr std::uint8_t element_count_in_word() const { return this->as_matrix().m_data.element_count_in_word(); }
		constexpr t_word element_mask() const { return this->as_matrix().m_data.element_mask(); }
	};
	
	
	template <typename t_matrix, typename t_word>
	struct int_matrix_width <t_matrix, 0, t_word> : public int_matrix_width_base <t_matrix>
	{
		std::uint8_t element_count_in_word() const { return this->as_matrix().m_data.element_count_in_word(); }
		t_word element_mask() const { return this->as_matrix().m_data.element_mask(); }
	};
	
	
	// Helpers for making it easier to pass the matrix type to the base class.
	template <unsigned int t_bits, typename t_word, template <typename, unsigned int, typename> typename t_trait>
	using int_matrix_trait_t = t_trait <int_matrix_tpl <t_bits, t_word, t_trait>, t_bits, t_word>;

	template <unsigned int t_bits, typename t_word, template <typename, unsigned int, typename> typename t_trait>
	using int_matrix_width_t = int_matrix_width <int_matrix_tpl <t_bits, t_word, t_trait>, t_bits, t_word>;
}}


namespace libbio {
	
	template <unsigned int t_bits, typename t_word, template <typename, unsigned int, typename> typename t_trait>
	class int_matrix_tpl final :
		public detail::int_matrix_trait_t <t_bits, t_word, t_trait>,
		public detail::int_matrix_width_t <t_bits, t_word, t_trait>
	{
		template <typename, unsigned int, typename>
		friend struct detail::int_matrix_trait;
		
		template <typename, unsigned int, typename>
		friend struct detail::atomic_int_matrix_trait;
		
		template <typename, unsigned int, typename>
		friend struct detail::int_matrix_width;
		
		template <typename t_matrix>
		friend std::size_t detail::matrix_index(t_matrix const &, std::size_t const, std::size_t const);
		
		template <typename t_matrix>
		friend std::size_t detail::matrix_index(t_matrix const &, std::size_t const, std::size_t const);
		
		template <typename t_archive, unsigned int t_bits_1, typename t_word_1, template <typename, unsigned int, typename> typename t_trait_1>
		friend void save(t_archive &, int_matrix_tpl <t_bits_1, t_word_1, t_trait_1> const &);
		
		template <typename t_archive, unsigned int t_bits_1, typename t_word_1, template <typename, unsigned int, typename> typename t_trait_1>
		friend void load(t_archive &, int_matrix_tpl <t_bits_1, t_word_1, t_trait_1> &);
		
	public:
		typedef detail::int_matrix_trait_t <t_bits, t_word, t_trait>				trait_type;
		typedef typename trait_type::vector_type									vector_type;
		typedef typename trait_type::word_type										word_type;
		typedef typename trait_type::reference_proxy								reference_proxy;
		
		typedef word_type															value_type;
		typedef reference_proxy														reference;
		typedef word_type															const_reference;
		
		typedef typename vector_type::iterator										iterator;
		typedef typename vector_type::const_iterator								const_iterator;
		typedef typename vector_type::word_iterator									word_iterator;
		typedef typename vector_type::const_word_iterator							const_word_iterator;
		typedef detail::int_matrix_iterator <vector_type>							matrix_iterator;
		typedef detail::int_matrix_iterator <vector_type const>						const_matrix_iterator;
		typedef detail::int_vector_word_iterator_proxy <vector_type>				word_iterator_proxy;
		typedef detail::int_vector_word_iterator_proxy <vector_type const>			const_word_iterator_proxy;
		typedef detail::int_vector_reverse_word_iterator_proxy <vector_type>		reverse_word_iterator_proxy;
		typedef detail::int_vector_reverse_word_iterator_proxy <vector_type const>	const_reverse_word_iterator_proxy;

		typedef detail::int_matrix_slice <int_matrix_tpl>							slice_type;
		typedef detail::int_matrix_slice <int_matrix_tpl const>						const_slice_type;
		
		friend trait_type;
		friend matrix_iterator;
		friend const_matrix_iterator;
		
		enum {
			WORD_BITS		= vector_type::WORD_BITS,
			ELEMENT_BITS	= vector_type::ELEMENT_BITS
		};
		
	protected:
		vector_type											m_data;
#ifndef LIBBIO_NDEBUG
		std::size_t											m_columns{};
#endif
		std::size_t											m_stride{1};
		
	public:
		int_matrix_tpl() = default;
		
		template <bool t_dummy = true, std::enable_if_t <t_dummy && 0 != t_bits, int> = 0>
		int_matrix_tpl(std::size_t const rows, std::size_t const columns):
			m_data(columns * rows),
#ifndef LIBBIO_NDEBUG
			m_columns(columns),
#endif
			m_stride(rows)
		{
			libbio_assert(m_stride);
		}
		
		// Enable even for 0 == t_bits in order to make writing constructors simpler in classes that make use of int_matrix.
		int_matrix_tpl(std::size_t const rows, std::size_t const columns, std::uint8_t const bits):
			m_data(columns * rows, bits),
#ifndef LIBBIO_NDEBUG
			m_columns(columns),
#endif
			m_stride(rows)
		{
			libbio_assert(m_stride);
		}
		
		inline std::size_t idx(std::size_t const y, std::size_t const x) const { return detail::matrix_index(*this, y, x); }
		
		std::size_t size() const { return m_data.size(); }
		std::size_t reserved_size() const { return m_data.reserved_size(); }
		std::size_t word_size() const { return m_data.word_size(); }
		void set_size(std::size_t new_size) { m_data.set_size(new_size); }
		std::size_t const number_of_columns() const { return m_data.size() / m_stride; }
		std::size_t const number_of_rows() const { return m_stride; }
		std::size_t const stride() const { return m_stride; }
		void set_stride(std::size_t const stride) { m_stride = stride; }
		constexpr std::size_t word_bits() const { return m_data.word_bits(); }
		
		vector_type const &values() const { return m_data; }
		
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
		reverse_word_iterator_proxy reverse_word_range() { return reverse_word_iterator_proxy(m_data); }
		const_reverse_word_iterator_proxy reverse_word_range() const { return const_reverse_word_iterator_proxy(m_data); }
		const_reverse_word_iterator_proxy reverse_const_word_range() const { return const_reverse_word_iterator_proxy(m_data); }
		
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
		
		bool operator==(int_matrix_tpl const &other) const;
	};
}


namespace libbio { namespace detail {
	
	template <typename t_matrix, unsigned int t_bits, typename t_word>
	auto atomic_int_matrix_trait <t_matrix, t_bits, t_word>::load(std::size_t const y, std::size_t const x, std::memory_order order) const -> word_type
	{
		auto &self(as_matrix());
		return self.m_data.load(self.idx(y, x), order);
	}
	
	
	template <typename t_matrix, unsigned int t_bits, typename t_word>
	auto atomic_int_matrix_trait <t_matrix, t_bits, t_word>::fetch_or(std::size_t const y, std::size_t const x, std::memory_order order) const -> word_type
	{
		auto &self(as_matrix());
		return self.m_data.fetch_or(self.idx(y, x), order);
	}
	
	
	template <typename t_matrix, unsigned int t_bits, typename t_word>
	auto atomic_int_matrix_trait <t_matrix, t_bits, t_word>::operator()(std::size_t const y, std::size_t const x) -> reference_proxy
	{
		auto &self(as_matrix());
		return self.m_data(self.idx(y, x));
	}
}}


namespace libbio {
	
	template <unsigned int t_bits, typename t_word = std::uint64_t>
	using int_matrix = int_matrix_tpl <t_bits, t_word, detail::int_matrix_trait>;
	
	template <unsigned int t_bits, typename t_word = std::uint64_t>
	using atomic_int_matrix = int_matrix_tpl <t_bits, t_word, detail::atomic_int_matrix_trait>;
	
	
	template <unsigned int t_bits, typename t_word, template <typename, unsigned int, typename> typename t_trait>
	bool int_matrix_tpl <t_bits, t_word, t_trait>::operator==(int_matrix_tpl const &other) const
	{
		return m_data == other.m_data && m_stride == other.m_stride;
	}
	
	
	template <unsigned int t_bits, typename t_word, template <typename, unsigned int, typename> typename t_trait>
	std::ostream &operator<<(std::ostream &os, int_matrix_tpl <t_bits, t_word, t_trait> const &matrix)
	{
		auto const row_count(matrix.number_of_rows());
		for (std::size_t i(0); i < row_count; ++i)
		{
			auto const &row(matrix.row(i));
			ranges::copy(row | ranges::views::transform([](auto val){ return +val; }), ranges::make_ostream_joiner(os, "\t"));
			os << '\n';
		}
		return os;
	}
}

#endif

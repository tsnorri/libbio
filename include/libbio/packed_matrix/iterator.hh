/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_PACKED_MATRIX_ITERATOR_HH
#define LIBBIO_PACKED_MATRIX_ITERATOR_HH

#include <libbio/packed_vector/iterator.hh>


namespace libbio { namespace detail {
	
	template <typename t_vector>
	class packed_matrix_iterator final :
		public packed_vector_iterator_base <t_vector>,
		public boost::iterator_facade <
			packed_matrix_iterator <t_vector>,
			typename packed_vector_iterator_traits <t_vector>::reference,
			boost::random_access_traversal_tag,
			typename packed_vector_iterator_traits <t_vector>::reference
		>
	{
		friend class boost::iterator_core_access;
		
	protected:
		typedef packed_vector_iterator_base <t_vector>	iterator_base;
		
	public:
		typedef packed_vector_iterator <t_vector> 		vector_iterator;
		
	protected:
		std::ptrdiff_t m_stride{1}; // Needs to be signed b.c. used in a division below.
		
	public:
		packed_matrix_iterator() = default;
		
		template <typename t_matrix>
		packed_matrix_iterator(t_matrix &matrix, std::size_t idx, std::size_t stride):
			iterator_base(matrix.m_data, idx),
			m_stride(stride)
		{
		}
		
		template <
			typename t_other_vector,
			typename = std::enable_if_t <std::is_convertible_v <t_other_vector *, t_vector *>>
		> packed_matrix_iterator(packed_matrix_iterator <t_other_vector> const &other):
			iterator_base(other.m_vector, other.m_idx),
			m_stride(other.m_stride)
		{
		}
		
		vector_iterator to_vector_iterator() const { return vector_iterator(*this->m_vector, this->m_idx); }
		
	private:
		void advance(std::ptrdiff_t const diff) override { this->m_idx += diff * m_stride; }
		bool equal(packed_matrix_iterator const &other) const { return iterator_base::equal(other) && m_stride == other.m_stride; }

		std::ptrdiff_t distance_to(packed_matrix_iterator const &other) const
		{
			auto const dist(iterator_base::distance_to(other));
			libbio_assert(0 == std::abs(dist) % m_stride);
			auto const retval(dist / m_stride);
			return retval;
		}
	};
}}

#endif

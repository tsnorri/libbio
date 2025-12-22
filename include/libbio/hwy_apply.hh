/*
 * Copyright (c) 2025 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_HWY_APPLY_HH
#define LIBBIO_HWY_APPLY_HH

#include <cstddef>
#include <type_traits>
#include <hwy/highway.h>
#include <hwy/aligned_allocator.h>

namespace libbio {

	// Helper for use with Google’s Highway library.
	template <typename t_tag>
	struct hwy_apply
	{
		typedef t_tag tag_type;
		typedef hwy::HWY_NAMESPACE::TFromD <tag_type> value_type;
		typedef hwy::HWY_NAMESPACE::VFromD <tag_type> vector_type;

		constexpr static tag_type const dd{};

#if HWY_HAVE_CONSTEXPR_LANES
		constexpr static std::size_t lanes{hwy::HWY_NAMESPACE::Lanes(dd)}; // Guaranteed to be a power of two (even on SVE).

		constexpr hwy_apply() = default;
#else
		std::size_t lanes{hwy::HWY_NAMESPACE::Lanes(dd)};
#endif

		HWY_ATTR static auto set(value_type val) { return hwy::HWY_NAMESPACE::Set(dd, val); }
		HWY_ATTR static void store_(vector_type const vec, value_type *dst) { hwy::HWY_NAMESPACE::Store(vec, dd, dst); }
		HWY_ATTR static void store_unaligned_(vector_type const vec, value_type *dst) { hwy::HWY_NAMESPACE::StoreU(vec, dd, dst); }

		template <typename t_limit>
		struct callback
		{
			t_limit ii{};

#if HWY_HAVE_CONSTEXPR_LANES
			constexpr static t_limit count() { return lanes; }
#else
			std::size_t lanes{hwy::HWY_NAMESPACE::Lanes(dd)};
			t_limit count() const { return lanes; }
#endif

			constexpr bool is_handling_remaining() const { return false; }
			HWY_ATTR auto set(value_type val) const { return hwy_apply::set(val); }
			HWY_ATTR auto load(value_type const *src) const { return hwy::HWY_NAMESPACE::Load(dd, src + ii); }
			HWY_ATTR void store(vector_type const vec, value_type *dst) const { hwy_apply::store_(vec, dst + ii); }
			HWY_ATTR void store_(vector_type const vec, value_type *dst) const { hwy_apply::store_(vec, dst); }
			HWY_ATTR void store_unaligned(vector_type const vec, value_type *dst) const { hwy_apply::store_unaligned_(vec, dst + ii); }
			HWY_ATTR void store_unaligned_(vector_type const vec, value_type *dst) const { hwy_apply::store_unaligned_(vec, dst); }
		};

		template <typename t_limit>
		struct remaining_callback {
			t_limit ii;
			t_limit remaining;

			remaining_callback(t_limit ii_, t_limit limit_):
				ii{ii_},
				remaining(limit_ - ii_)
			{
			}

			operator bool() const { return remaining; }
			t_limit count() const { return remaining; }
			constexpr bool is_handling_first() const { return false; }
			constexpr bool is_handling_remaining() const { return true; }
			HWY_ATTR auto set(value_type val) const { return hwy_apply::set(val); }
			HWY_ATTR auto load(value_type const *src) const { return hwy::HWY_NAMESPACE::LoadN(dd, src + ii, remaining); }
			HWY_ATTR void store(vector_type const vec, value_type *dst) const { hwy::HWY_NAMESPACE::StoreN(vec, dd, dst + ii, remaining); }
			HWY_ATTR void store_(vector_type const vec, value_type *dst) const { hwy::HWY_NAMESPACE::StoreN(vec, dd, dst, remaining); }
			HWY_ATTR void store_unaligned(vector_type const vec, value_type *dst) const { hwy::HWY_NAMESPACE::StoreN(vec, dd + ii, dst, remaining); }
			HWY_ATTR void store_unaligned_(vector_type const vec, value_type *dst) const { hwy::HWY_NAMESPACE::StoreN(vec, dd, dst, remaining); }
		};


		template <typename t_limit, typename t_fn>
		[[clang::always_inline]]
		[[gnu::always_inline]]
		HWY_ATTR void handle_remaining(t_limit ii, t_limit const limit, t_fn &&fn) const
		{
			remaining_callback callback{ii, limit};
			if (callback)
				fn(callback);
		}


		template <typename t_limit, bool t_needs_to_check_remaining, typename t_fn>
		HWY_ATTR void operator()(t_limit const limit, std::bool_constant <t_needs_to_check_remaining>, t_fn &&fn) const
		{
			auto const pos{[&] -> t_limit {
				callback <t_limit> callback{};

				while (callback.ii + lanes <= limit)
				{
					fn(callback);
					callback.ii += lanes;
				}

				return callback.ii;
			}()};

			if constexpr (t_needs_to_check_remaining)
				handle_remaining(pos, limit, fn);
		}


		template <typename t_limit, typename t_fn>
		HWY_ATTR void operator()(t_limit const limit, t_fn &&fn) const
		{
			operator()(limit, std::true_type{}, std::forward <t_fn>(fn));
		}
	};
}

#endif

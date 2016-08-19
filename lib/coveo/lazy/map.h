// Copyright (c) 2015-2016, Coveo Solutions Inc.
// Distributed under the Apache License, Version 2.0 (see LICENSE).

// Definitions of coveo::lazy::map and coveo::lazy::multimap.
// See the lazy_sorted_container.h header for actual implementation.

#ifndef COVEO_LAZY_MAP_H
#define COVEO_LAZY_MAP_H

#include <coveo/lazy/detail/lazy_sorted_container.h>

#include <vector>

namespace coveo {
namespace lazy {

// Default allocator type for lazy maps. Use this to specify a custom allocator
// for lazy maps or lazy multimaps:
//
//   // my_custom_allocator must be a template accepting a single type argument
//   coveo::lazy::map_allocator<key_type, mapped_type, my_custom_allocator>
//
// By default, the allocator type is std::allocator.
template<typename K,
         typename T,
         template<typename _AllocT> typename _Alloc = std::allocator>
using map_allocator = detail::map_allocator<K, T, _Alloc>;

// std::map-like container class that stores its elements in an internal container
// (by default, an std::vector) and sorts them only when needed. Sorting can also
// be triggered on-demand.
//
// See detail::lazy_sorted_container for details on the actual implementation.
//
// This class has the following differences compared to std::map:
// - Because it uses a sequence container internally, elements in the map might
//   be moved/copied when an insertion occurs. Thus, this class cannot be used
//   to store move-only types.
// - It needs a predicate to determine if two map keys are equal. If an operator==
//   exists to compare instances of type K, it will be used; otherwise, it will
//   use an implementation that uses key_compare to determine equality. This
//   is less efficient; if a custom equality predicate exists for type K, it
//   should be used for the _Eq template parameter.
// - Modifier methods do not return iterators, except those that require the
//   map to be sorted (e.g., insert_or_assign(), try_emplace()). This is done
//   to better support lazy sorting.
// - Methods that require the map to be sorted (e.g. operator[], at(),
//   insert_or_assign(), try_emplace()) are less efficient because we cannot
//   take advantage of lazy sorting.
template<typename K,
         typename T,
         typename _Cmp = std::less<K>,
         template<typename _ImplT, typename _ImplAlloc> typename _Impl = std::vector,
         typename _Eq = detail::equal_to_using_less_if_needed<K, _Cmp>,
         typename _Alloc = map_allocator<K, T>>
 using map = detail::lazy_sorted_container<K,
                                           T,
                                           detail::map_pair<K, T>,
                                           typename detail::map_pair<K, T>::base_std_pair,
                                           detail::pair_first<detail::map_pair<K, T>>,
                                           _Cmp,
                                           _Eq,
                                           _Alloc,
                                           _Impl,
                                           false>;

// Equivalent to the type above, but supports duplicate elements like std::multimap.
template<typename K,
         typename T,
         typename _Cmp = std::less<K>,
         template<typename _ImplT, typename _ImplAlloc> typename _Impl = std::vector,
         typename _Eq = detail::equal_to_using_less_if_needed<K, _Cmp>,
         typename _Alloc = map_allocator<K, T>>
 using multimap = detail::lazy_sorted_container<K,
                                                T,
                                                detail::map_pair<K, T>,
                                                typename detail::map_pair<K, T>::base_std_pair,
                                                detail::pair_first<detail::map_pair<K, T>>,
                                                _Cmp,
                                                _Eq,
                                                _Alloc,
                                                _Impl,
                                                true>;

} // lazy
} // coveo

#endif // COVEO_LAZY_MAP_H

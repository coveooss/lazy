// Copyright (c) 2015-2016, Coveo Solutions Inc.
// Distributed under the Apache License, Version 2.0 (see LICENSE).

// Definitions of coveo::lazy::set and coveo::lazy::multiset.
// See the lazy_sorted_container.h header for actual implementation.

#ifndef COVEO_LAZY_SET_H
#define COVEO_LAZY_SET_H

#include <coveo/lazy/detail/lazy_sorted_container.h>

#include <vector>

namespace coveo {
namespace lazy {

// std::set-like container class that stores its elements in an internal container
// (by default, an std::vector) and sorts them only when needed. Sorting can also
// be triggered on-demand.
//
// See detail::lazy_sorted_container for details on the actual implementation.
//
// This class has the following differences compared to std::set:
// - Because it uses a sequence container internally, elements in the set might
//   be moved/copied when an insertion occurs. Thus, this class cannot be used
//   to store move-only types.
// - It needs a predicate to determine if two set elements are equal. If an operator==
//   exists to compare instances of type K, it will be used; otherwise, it will
//   use an implementation that uses key_compare to determine equality. This
//   is less efficient; if a custom equality predicate exists for type K, it
//   should be used for the _Eq template parameter.
// - Modifier methods do not return iterators. This is done to better support
//   lazy sorting.
template<typename K,
         typename _Cmp = std::less<K>,
         template<typename _ImplT, typename _ImplAlloc> typename _Impl = std::vector,
         typename _Eq = detail::equal_to_using_less_if_needed<K, _Cmp>,
         typename _Alloc = std::allocator<K>>
 using set = detail::lazy_sorted_container<K,
                                           void,
                                           K,
                                           K,
                                           detail::identity<K>,
                                           _Cmp,
                                           _Eq,
                                           _Alloc,
                                           _Impl,
                                           false>;

// Equivalent to the type above, but supports duplicate elements like std::multiset.
template<typename K,
         typename _Cmp = std::less<K>,
         template<typename _ImplT, typename _ImplAlloc> typename _Impl = std::vector,
         typename _Eq = detail::equal_to_using_less_if_needed<K, _Cmp>,
         typename _Alloc = std::allocator<K>>
 using multiset = detail::lazy_sorted_container<K,
                                                void,
                                                K,
                                                K,
                                                detail::identity<K>,
                                                _Cmp,
                                                _Eq,
                                                _Alloc,
                                                _Impl,
                                                true>;

} // lazy
} // coveo

#endif // COVEO_LAZY_SET_H

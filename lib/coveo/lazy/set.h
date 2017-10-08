/**
 * @file
 * @brief Definition of lazy-sorted set-like containers.
 *
 * This file contains definitions for <tt>coveo::lazy::set</tt>
 * and <tt>coveo::lazy::multiset</tt>. These lazy-sorted containers
 * mimic the standard C++ containers of the same name, but perform
 * lazy sorting.
 *
 * These definitions are just typedefs. For actual implementation,
 * see <tt>coveo/lazy/detail/lazy_sorted_container.h</tt>.
 *
 * @copyright 2015-2016, Coveo Solutions Inc.
 *            Distributed under the Apache License, Version 2.0 (see <a href="https://github.com/coveo/lazy/blob/master/LICENSE">LICENSE</a>).
 */

#ifndef COVEO_LAZY_SET_H
#define COVEO_LAZY_SET_H

#include <coveo/lazy/detail/lazy_sorted_container.h>

#include <vector>

namespace coveo {
namespace lazy {

/**
 * @class coveo::lazy::set
 * @extends coveo::lazy::detail::lazy_sorted_container
 * @brief Set container that performs lazy sorting.
 * @headerfile set.h <coveo/lazy/set.h>
 *
 * <tt>std::set</tt>-like container class that stores its elements in an internal
 * container (by default, an <tt>std::vector</tt>) and sorts them only when needed.
 * Sorting can also be triggered on-demand.
 *
 * See <tt>coveo::lazy::detail::lazy_sorted_container</tt> for details
 * on the actual implementation.
 *
 * This class has the following differences compared to <tt>std::set</tt>:
 *
 * - Because it uses a sequence container internally, elements in the set might
 *   be moved/copied when an insertion occurs. Thus, this class cannot be used
 *   to store move-only types. Furthermore, inserting an element invalidates
 *   all iterators and references.
 * - It needs a predicate to determine if two set elements are equal. If an
 *   <tt>operator==</tt> exists to compare instances of type @c K, it will be
 *   used; otherwise, it will use an implementation that uses @c key_compare
 *   to determine equality. This is less efficient; if a custom equality
 *   predicate exists for type @c K, it should be used for the @c _Eq template
 *   parameter.
 * - Modifier methods do not return iterators. This is done to better
 *   support lazy sorting.
 *
 * @tparam K Type of elements stored in the set.
 * @tparam _Cmp Predicate used to sort the elements.
 *              Defaults to <tt>std::less<K></tt>.
 * @tparam _Impl Type of sequence container used by the set.
 *               Defaults to <tt>std::vector</tt>.
 * @tparam _Eq Predicate used to determine if elements are equal.
 *             Defaults to using <tt>operator==</tt> if available,
 *             otherwise to an implementation that uses @c _Cmp (see above).
 * @tparam _Alloc Allocator used for the set elements.
 *                Defaults to <tt>std::allocator</tt>.
 */
template<class K,
         class _Cmp = std::less<K>,
         template<class _ImplT, class _ImplAlloc> class _Impl = std::vector,
         class _Eq = detail::equal_to_using_less_if_needed<K, _Cmp>,
         class _Alloc = std::allocator<K>>
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

/**
 * @class coveo::lazy::multiset
 * @extends coveo::lazy::detail::lazy_sorted_container
 * @brief Multiset container that performs lazy sorting.
 * @headerfile set.h <coveo/lazy/set.h>
 *
 * <tt>std::multiset</tt>-like container class that stores its elements in an internal
 * container (by default, an <tt>std::vector</tt>) and sorts them only when needed.
 * Sorting can also be triggered on-demand.
 *
 * Similar to <tt>coveo::lazy::set</tt> but accepts duplicate elements.
 * Duplicates will appear in insertion order when iterated.
 *
 * See <tt>coveo::lazy::detail::lazy_sorted_container</tt> for details
 * on the actual implementation.
 *
 * This class has the following differences compared to <tt>std::multiset</tt>:
 *
 * - Because it uses a sequence container internally, elements in the set might
 *   be moved/copied when an insertion occurs. Thus, this class cannot be used
 *   to store move-only types. Furthermore, inserting an element invalidates
 *   all iterators and references.
 * - It needs a predicate to determine if two set elements are equal. If an
 *   <tt>operator==</tt> exists to compare instances of type @c K, it will be
 *   used; otherwise, it will use an implementation that uses @c key_compare
 *   to determine equality. This is less efficient; if a custom equality
 *   predicate exists for type @c K, it should be used for the @c _Eq template
 *   parameter.
 * - Modifier methods do not return iterators. This is done to better
 *   support lazy sorting.
 *
 * @tparam K Type of elements stored in the set.
 * @tparam _Cmp Predicate used to sort the elements.
 *              Defaults to <tt>std::less<K></tt>.
 * @tparam _Impl Type of sequence container used by the set.
 *               Defaults to <tt>std::vector</tt>.
 * @tparam _Eq Predicate used to determine if elements are equal.
 *             Defaults to using <tt>operator==</tt> if available,
 *             otherwise to an implementation that uses @c _Cmp (see above).
 * @tparam _Alloc Allocator used for the set elements.
 *                Defaults to <tt>std::allocator</tt>.
 */
template<class K,
         class _Cmp = std::less<K>,
         template<class _ImplT, class _ImplAlloc> class _Impl = std::vector,
         class _Eq = detail::equal_to_using_less_if_needed<K, _Cmp>,
         class _Alloc = std::allocator<K>>
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

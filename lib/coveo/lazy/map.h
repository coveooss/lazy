/**
 * @file
 * @brief Definition of lazy-sorted map-like containers.
 *
 * This file contains definitions for <tt>coveo::lazy::map</tt>
 * and <tt>coveo::lazy::multimap</tt>. These lazy-sorted containers
 * mimic the standard C++ containers of the same name, but perform
 * lazy sorting.
 *
 * These definitions are just typedefs. For actual implementation,
 * see <tt>coveo/lazy/detail/lazy_sorted_container.h</tt>.
 *
 * @copyright 2015-2016, Coveo Solutions Inc.
 *            Distributed under the Apache License, Version 2.0 (see <a href="https://github.com/coveo/lazy/blob/master/LICENSE">LICENSE</a>).
 */

#ifndef COVEO_LAZY_MAP_H
#define COVEO_LAZY_MAP_H

#include <coveo/lazy/detail/lazy_sorted_container.h>

#include <vector>

namespace coveo {
namespace lazy {

/**
 * @brief Allocator for lazy maps.
 *
 * Default allocator type for lazy maps. Use this to specify a custom allocator
 * for <tt>coveo::lazy::map</tt> or <tt>coveo::lazy::multimap</tt>:
 *
 * @code
 *   // my_custom_allocator must be a template accepting a single type argument
 *   coveo::lazy::map_allocator<key_type, mapped_type, my_custom_allocator>
 * @endcode
 *
 * @tparam K Type of keys stored in the map.
 * @tparam T Type of values stored in the map.
 * @tparam _Alloc Base allocator actually used to perform allocation. Must be
 *                a template type accepting a single argument. By default,
 *                the allocator type is <tt>std::allocator</tt>.
 */
template<typename K,
         typename T,
         template<typename _AllocT> typename _Alloc = std::allocator>
using map_allocator = detail::map_allocator<K, T, _Alloc>;

/**
 * @class coveo::lazy::map
 * @extends coveo::lazy::detail::lazy_sorted_container
 * @brief Map container that performs lazy sorting.
 * @headerfile map.h <coveo/lazy/map.h>
 *
 * <tt>std::map</tt>-like container class that stores its elements in an internal
 * container (by default, an <tt>std::vector</tt>) and sorts them only when needed.
 * Sorting can also be triggered on-demand.
 *
 * See <tt>coveo::lazy::detail::lazy_sorted_container</tt> for details
 * on the actual implementation.
 *
 * This class has the following differences compared to <tt>std::map</tt>:
 *
 * - Because it uses a sequence container internally, elements in the map might
 *   be moved/copied when an insertion occurs. Thus, this class cannot be used
 *   to store move-only types.
 * - It needs a predicate to determine if two map keys are equal. If an
 *   <tt>operator==</tt> exists to compare instances of type @c K, it will be
 *   used; otherwise, it will use an implementation that uses @c key_compare
 *   to determine equality. This is less efficient; if a custom equality
 *   predicate exists for type @c K, it should be used for the @c _Eq template
 *   parameter.
 * - Modifier methods do not return iterators, except those that require the
 *   map to be sorted (e.g., <tt>insert_or_assign()</tt>, <tt>try_emplace()</tt>).
 *   This is done to better support lazy sorting.
 * - Methods that require the map to be sorted (e.g. <tt>operator[]</tt>,
 *   <tt>at()</tt>, <tt>insert_or_assign()</tt>, <tt>try_emplace()</tt>) are
 *   less efficient because we cannot take advantage of lazy sorting.
 *
 * @tparam K Type of keys stored in the map.
 * @tparam T Type of values stored in the map.
 * @tparam _Cmp Predicate used to sort the keys.
 *              Defaults to <tt>std::less<K></tt>.
 * @tparam _Impl Type of sequence container used by the map.
 *               Defaults to <tt>std::vector</tt>.
 * @tparam _Eq Predicate used to determine if keys are equal.
 *             Defaults to using <tt>operator==</tt> if available,
 *             otherwise to an implementation that uses @c _Cmp (see above).
 * @tparam _Alloc Allocator used for the map elements.
 *                Defaults to @c map_allocator.
 */
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

/**
 * @class coveo::lazy::multimap
 * @extends coveo::lazy::detail::lazy_sorted_container
 * @brief Multimap container that performs lazy sorting.
 * @headerfile map.h <coveo/lazy/map.h>
 *
 * <tt>std::multimap</tt>-like container class that stores its elements in an internal
 * container (by default, an <tt>std::vector</tt>) and sorts them only when needed.
 * Sorting can also be triggered on-demand.
 *
 * Similar to <tt>coveo::lazy::map</tt> but accepts duplicate elements.
 * Duplicates will appear in insertion order when iterated.
 *
 * See <tt>coveo::lazy::detail::lazy_sorted_container</tt> for details
 * on the actual implementation.
 *
 * This class has the following differences compared to <tt>std::multimap</tt>:
 *
 * - Because it uses a sequence container internally, elements in the map might
 *   be moved/copied when an insertion occurs. Thus, this class cannot be used
 *   to store move-only types.
 * - It needs a predicate to determine if two map keys are equal. If an
 *   <tt>operator==</tt> exists to compare instances of type @c K, it will be
 *   used; otherwise, it will use an implementation that uses @c key_compare
 *   to determine equality. This is less efficient; if a custom equality
 *   predicate exists for type @c K, it should be used for the @c _Eq template
 *   parameter.
 * - Modifier methods do not return iterators. This is done to better
 *   support lazy sorting.
 *
 * @tparam K Type of keys stored in the map.
 * @tparam T Type of values stored in the map.
 * @tparam _Cmp Predicate used to sort the keys.
 *              Defaults to <tt>std::less<K></tt>.
 * @tparam _Impl Type of sequence container used by the map.
 *               Defaults to <tt>std::vector</tt>.
 * @tparam _Eq Predicate used to determine if keys are equal.
 *             Defaults to using <tt>operator==</tt> if available,
 *             otherwise to an implementation that uses @c _Cmp (see above).
 * @tparam _Alloc Allocator used for the map elements.
 *                Defaults to @c map_allocator.
 */
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

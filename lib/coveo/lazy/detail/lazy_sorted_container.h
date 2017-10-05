/**
 * @file
 * @brief Lazy-sorted container internals.
 *
 * This header file contains implementation details of lazy-sorted
 * associative containers. It should not be necessary to use types
 * defined in this header directly; instead, use types declared in
 * <tt>coveo/lazy/map.h</tt> or <tt>coveo/lazy/set.h</tt>.
 *
 * @copyright 2015-2016, Coveo Solutions Inc.
 *            Distributed under the Apache License, Version 2.0 (see <a href="https://github.com/coveo/lazy/blob/master/LICENSE">LICENSE</a>).
 */

/**
 * @mainpage <tt>coveo::lazy</tt>
 *
 * Welcome to the documentation of the <tt>coveo::lazy</tt> library.
 * This library implements four associative containers that are similar
 * (in terms of interface) to the associative containers provided by the
 * standard C++ library. The main difference is their implementation:
 * instead of storing elements in a tree-like structure, they store
 * their elements in a regular container (by default <tt>std::vector</tt>)
 * and perform sorting only when needed (when @c find is called, for
 * instance). This results in dramatically-reduced storage space, at
 * the cost of an expensive sorting operation once in a while.
 *
 * These containers are designed to be used in situations where insertions
 * in the container are usually performed in batches, followed by queries.
 * They are simpler to use than manually sorting <tt>std::vector</tt>s.
 *
 * Here is an example that uses <tt>coveo::lazy::set</tt>:
 *
 * @code
 *   // Declare our set
 *   coveo::lazy::set<std::string> our_set;
 *
 *   // Populate it in batch. This is more efficient than std::set.
 *   for (const auto& s : some_sequence) {
 *       our_set.insert(s);
 *   }
 *
 *   // Now we're ready to perform queries. The first one
 *   // will trigger a sorting of the set's elements.
 *   for (;;) {
 *       std::cout << "Enter a string: ";
 *       std::string s;
 *       std::getline(std::cin, s);
 *       if (s.empty()) {
 *           break;
 *       } else if (our_set.find(s) != our_set.end()) {
 *           std::cout << "String was found!" << std::endl;
 *       } else {
 *           std::cout << "Not found." << std::endl;
 *       }
 *   }
 * @endcode
 *
 * @copyright 2015-2016, Coveo Solutions Inc.
 *            Distributed under the Apache License, Version 2.0 (see <a href="https://github.com/coveo/lazy/blob/master/LICENSE">LICENSE</a>).
 */

#ifndef COVEO_LAZY_DETAIL_LAZY_SORTED_CONTAINER_H
#define COVEO_LAZY_DETAIL_LAZY_SORTED_CONTAINER_H

#include <coveo/lazy/exception.h>

#include <algorithm>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>

namespace coveo {
namespace lazy {
namespace detail {

/**
 * @internal
 * @brief Incomplete type to make some template usage fail.
 * @headerfile lazy_sorted_container.h <coveo/lazy/detail/lazy_sorted_container.h>
 */
struct incomplete_type;

/**
 * @internal
 * @brief Trait to detect <tt>operator==</tt>.
 * @headerfile lazy_sorted_container.h <coveo/lazy/detail/lazy_sorted_container.h>
 *
 * Type trait that can be used to know if there is a usable
 * <tt>operator==</tt> for a given type. Detects both member
 * and non-member functions. Inspired by
 * http://stackoverflow.com/a/257382 and others.
 */
template<class T>
class has_operator_equal
{
    static_assert(sizeof(std::int_least8_t) != sizeof(std::int_least32_t),
                  "has_operator_equal only works if int_least8_t has a different size than int_least32_t");

    template<class C> static std::int_least8_t  test(decltype(std::declval<C>() == std::declval<C>())*); // Will be selected if operator== exists for C
    template<class C> static std::int_least32_t test(...);                                               // Will be selected otherwise
public:
    static const bool value = sizeof(test<T>(nullptr)) == sizeof(std::int_least8_t);
};

/**
 * @internal
 * @brief Trait to detect @c reserve method.
 * @headerfile lazy_sorted_container.h <coveo/lazy/detail/lazy_sorted_container.h>
 *
 * Type trait that can be used to know if a type has a
 * <tt>reserve()</tt> method that is callable with a
 * single argument of the given type.
 */
template<class T, class SizeT>
class has_reserve_method
{
    static_assert(sizeof(std::int_least8_t) != sizeof(std::int_least32_t),
                  "has_reserve_method only works if int_least8_t has a different size than int_least32_t");

    template<class C> static std::int_least8_t  test(decltype(std::declval<C>().reserve(std::declval<SizeT>()))*);   // Will be selected if C has reserve() accepting SizeT
    template<class C> static std::int_least32_t test(...);                                                           // Will be selected otherwise
public:
    static const bool value = sizeof(test<T>(nullptr)) == sizeof(std::int_least8_t);
};

/**
 * @internal
 * @brief Trait to detect @c capacity method.
 * @headerfile lazy_sorted_container.h <coveo/lazy/detail/lazy_sorted_container.h>
 *
 * Type trait that can be used to know if a type has a
 * <tt>capacity() const</tt> method that returns a
 * non-<tt>void</tt> result.
 */
template<class T>
class has_capacity_const_method
{
    static_assert(sizeof(std::int_least8_t) != sizeof(std::int_least32_t),
                  "has_capacity_method only works if int_least8_t has a different size than int_least32_t");

    template<class C> static std::int_least8_t  test(std::enable_if_t<!std::is_void<decltype(std::declval<const C>().capacity())>::value, void*>);   // Will be selected if C has capacity() that does not return void
    template<class C> static std::int_least32_t test(...);                                                                                           // Will be selected otherwise
public:
    static const bool value = sizeof(test<T>(nullptr)) == sizeof(std::int_least8_t);
};

/**
 * @internal
 * @brief Trait to detect @c shrink_to_fit method.
 * @headerfile lazy_sorted_container.h <coveo/lazy/detail/lazy_sorted_container.h>
 *
 * Type trait that can be used to know if a type has a
 * <tt>shrink_to_fit()</tt> method.
 */
template<class T>
class has_shrink_to_fit_method
{
    static_assert(sizeof(std::int_least8_t) != sizeof(std::int_least32_t),
                  "has_shrink_to_fit_method only works if int_least8_t has a different size than int_least32_t");

    template<class C> static std::int_least8_t  test(decltype(std::declval<C>().shrink_to_fit())*);  // Will be selected if C has shrink_to_fit()
    template<class C> static std::int_least32_t test(...);                                           // Will be selected otherwise
public:
    static const bool value = sizeof(test<T>(nullptr)) == sizeof(std::int_least8_t);
};

/**
 * @internal
 * @brief Predicate that proxies a key-based predicate.
 * @headerfile lazy_sorted_container.h <coveo/lazy/detail/lazy_sorted_container.h>
 *
 * Proxy predicate that can act on values in a lazy sorted container by using
 * a "value to key" function object to forward the values' keys to a key-based
 * predicate. Can also act on any object that can be interpreted as a "key" by
 * the key-based predicate.
 *
 * @tparam V Type of values passed to this predicate.
 * @tparam VToK Predicate that can extract a key for a value.
 * @tparam KPred Key-based predicate to proxy.
 */
template<class V,
         class VToK,
         class KPred>
class lazy_value_pred_proxy
{
    VToK vtok_;
    KPred kpr_;
public:
    template<class OVToK, class OKPred>
    lazy_value_pred_proxy(OVToK&& vtok, OKPred&& kpr)
        : vtok_(std::forward<OVToK>(vtok)), kpr_(std::forward<OKPred>(kpr)) { }

    KPred key_predicate() const {
        return kpr_;
    }

    // Act on two values using their keys
    decltype(auto) operator()(const V& left, const V& right) const {
        return kpr_(vtok_(left), vtok_(right));
    }

    // Act on a value and a key, or any object that the key-based predicate supports
    template<class OK,
             class = std::enable_if_t<!std::is_same<OK, V>::value && !std::is_base_of<V, OK>::value, void>>
    decltype(auto) operator()(const V& left, const OK& rightk) const {
        return kpr_(vtok_(left), rightk);
    }
    template<class OK,
             class = std::enable_if_t<!std::is_same<OK, V>::value && !std::is_base_of<V, OK>::value, void>>
    decltype(auto) operator()(const OK& leftk, const V& right) const {
        return kpr_(leftk, vtok_(right));
    }
};

/**
 * @internal
 * @brief Forward iterator that publishes a different reference type.
 * @headerfile lazy_sorted_container.h <coveo/lazy/detail/lazy_sorted_container.h>
 *
 * Forward iterator proxy that wraps another forward iterator but converts the
 * reference returned by <tt>operator*()</tt> to another reference type.
 *
 * @tparam It Type of iterator to proxy.
 * @tparam V Type of values returned by @c It.
 * @tparam PubV Type of values to publicly return references of.
 *              Must be either @c V or a base class of @c V.
 * @tparam _ItCat Iterator category for @c It.
 *                Uses <tt>std::iterator_traits</tt> by default.
 * @tparam _Diff Type used for differences between iterators.
 *               Uses <tt>std::iterator_traits</tt> by default.
 */
template<class It,
         class V,
         class PubV,
         class _ItCat = typename std::iterator_traits<It>::iterator_category,
         class _Diff = typename std::iterator_traits<It>::difference_type>
class forward_iterator_proxy : public std::iterator<_ItCat, PubV, _Diff, PubV*, PubV&>
{
    static_assert(std::is_base_of<std::forward_iterator_tag, _ItCat>::value,
                  "forward_iterator_proxy can only wrap forward iterators");
    static_assert(std::is_same<PubV, V>::value || std::is_base_of<PubV, V>::value,
                  "forward_iterator_proxy can only return references to base classes of their wrapped iterator's value_type");

    // Helper used to identify our type
    template<class T> struct is_forward_iterator_proxy : std::false_type { };
    template<class OIt, class OV, class OPubV, class _OItCat, class _ODiff>
    struct is_forward_iterator_proxy<forward_iterator_proxy<OIt, OV, OPubV, _OItCat, _ODiff>> : std::true_type { };
    
    // Friend compatible iterators to be able to access it_
    template<class OIt, class OV, class OPubV, class _OItCat, class _ODiff>
    friend class forward_iterator_proxy;
protected:
    It it_; // Iterator being proxied

public:
    forward_iterator_proxy() = default;
    forward_iterator_proxy(const forward_iterator_proxy& other) = default;
    forward_iterator_proxy(forward_iterator_proxy&& other) = default;
    template<class OIt, class OV, class OPubV, class _OItCat, class _ODiff>
    forward_iterator_proxy(const forward_iterator_proxy<OIt, OV, OPubV, _OItCat, _ODiff>& other)
        : it_(other.it_) { }
    template<class OIt, class OV, class OPubV, class _OItCat, class _ODiff>
    forward_iterator_proxy(forward_iterator_proxy<OIt, OV, OPubV, _OItCat, _ODiff>&& other)
        : it_(std::move(other.it_)) { }
    template<class T,
             class = std::enable_if_t<!is_forward_iterator_proxy<std::decay_t<T>>::value, void>>
    forward_iterator_proxy(T&& obj) : it_(std::forward<T>(obj)) { }
    template<class T1, class T2, class... Tx>
    forward_iterator_proxy(T1&& obj1, T2&& obj2, Tx&&... objx)
        : it_(std::forward<T1>(obj1), std::forward<T2>(obj2), std::forward<Tx>(objx)...) { }

    forward_iterator_proxy& operator=(const forward_iterator_proxy& other) = default;
    forward_iterator_proxy& operator=(forward_iterator_proxy&& other) = default;
    template<class OIt, class OV, class OPubV, class _OItCat, class _ODiff>
    forward_iterator_proxy& operator=(const forward_iterator_proxy<OIt, OV, OPubV, _OItCat, _ODiff>& other) {
        it_ = other.it_;
        return *this;
    }
    template<class OIt, class OV, class OPubV, class _OItCat, class _ODiff>
    forward_iterator_proxy& operator=(forward_iterator_proxy<OIt, OV, OPubV, _OItCat, _ODiff>&& other) {
        it_ = std::move(other.it_);
        return *this;
    }
    template<class T,
             class = std::enable_if_t<!is_forward_iterator_proxy<std::decay_t<T>>::value, void>>
    forward_iterator_proxy& operator=(T&& obj) {
        it_ = std::forward<T>(obj);
        return *this;
    }

    // Conversion function to our proxied iterator type. Must be implicit.
    // Necessary to make proxies usable when calling methods that accept
    // the proxied iterator type without explicit casts.
    operator const It&() const { return it_; }

    PubV& operator*() const { return *it_; }
    PubV* operator->() const { return &**this; }

    forward_iterator_proxy& operator++() {
        ++it_;
        return *this;
    }
    forward_iterator_proxy operator++(int) {
        forward_iterator_proxy oldit(*this);
        ++*this;
        return oldit;
    }

    friend decltype(auto) operator==(const forward_iterator_proxy& left, const forward_iterator_proxy& right) {
        return left.it_ == right.it_;
    }
    friend decltype(auto) operator!=(const forward_iterator_proxy& left, const forward_iterator_proxy& right) {
        return left.it_ != right.it_;
    }
};

/**
 * @internal
 * @brief Bidirectional iterator that publishes a different reference type.
 * @headerfile lazy_sorted_container.h <coveo/lazy/detail/lazy_sorted_container.h>
 *
 * Bidirectional iterator proxy that wraps another bidirectional iterator but
 * converts the reference returned by <tt>operator*()</tt> to another reference type.
 *
 * @tparam It Type of iterator to proxy.
 * @tparam V Type of values returned by @c It.
 * @tparam PubV Type of values to publicly return references of.
 *              Must be either @c V or a base class of @c V.
 * @tparam _ItCat Iterator category for @c It.
 *                Uses <tt>std::iterator_traits</tt> by default.
 * @tparam _Diff Type used for differences between iterators.
 *               Uses <tt>std::iterator_traits</tt> by default.
 */
template<class It,
         class V,
         class PubV,
         class _ItCat = typename std::iterator_traits<It>::iterator_category,
         class _Diff = typename std::iterator_traits<It>::difference_type>
class bidirectional_iterator_proxy : public forward_iterator_proxy<It, V, PubV, _ItCat, _Diff>
{
    static_assert(std::is_base_of<std::bidirectional_iterator_tag, _ItCat>::value,
                  "bidirectional_iterator_proxy can only wrap bidirectional iterators");

    // Helper used to identify our type
    template<class T> struct is_bidirectional_iterator_proxy : std::false_type { };
    template<class OIt, class OV, class OPubV, class _OItCat, class _ODiff>
    struct is_bidirectional_iterator_proxy<bidirectional_iterator_proxy<OIt, OV, OPubV, _OItCat, _ODiff>> : std::true_type { };
public:
    bidirectional_iterator_proxy() = default;
    bidirectional_iterator_proxy(const bidirectional_iterator_proxy& other) = default;
    bidirectional_iterator_proxy(bidirectional_iterator_proxy&& other) = default;
    template<class OIt, class OV, class OPubV, class _OItCat, class _ODiff>
    bidirectional_iterator_proxy(const bidirectional_iterator_proxy<OIt, OV, OPubV, _OItCat, _ODiff>& other)
        : forward_iterator_proxy<It, V, PubV, _ItCat, _Diff>(static_cast<const forward_iterator_proxy<OIt, OV, OPubV, _OItCat, _ODiff>&>(other)) { }
    template<class OIt, class OV, class OPubV, class _OItCat, class _ODiff>
    bidirectional_iterator_proxy(bidirectional_iterator_proxy<OIt, OV, OPubV, _OItCat, _ODiff>&& other)
        : forward_iterator_proxy<It, V, PubV, _ItCat, _Diff>(static_cast<forward_iterator_proxy<OIt, OV, OPubV, _OItCat, _ODiff>&&>(other)) { }
    template<class T,
             class = std::enable_if_t<!is_bidirectional_iterator_proxy<std::decay_t<T>>::value, void>>
    bidirectional_iterator_proxy(T&& obj)
        : forward_iterator_proxy<It, V, PubV, _ItCat, _Diff>(std::forward<T>(obj)) { }
    template<class T1, class T2, class... Tx>
    bidirectional_iterator_proxy(T1&& obj1, T2&& obj2, Tx&&... objx)
        : forward_iterator_proxy<It, V, PubV, _ItCat, _Diff>(std::forward<T1>(obj1), std::forward<T2>(obj2), std::forward<Tx>(objx)...) { }

    bidirectional_iterator_proxy& operator=(const bidirectional_iterator_proxy& other) = default;
    bidirectional_iterator_proxy& operator=(bidirectional_iterator_proxy&& other) = default;
    template<class OIt, class OV, class OPubV, class _OItCat, class _ODiff>
    bidirectional_iterator_proxy& operator=(const bidirectional_iterator_proxy<OIt, OV, OPubV, _OItCat, _ODiff>& other) {
        forward_iterator_proxy<It, V, PubV, _ItCat, _Diff>::operator=(other);
        return *this;
    }
    template<class OIt, class OV, class OPubV, class _OItCat, class _ODiff>
    bidirectional_iterator_proxy& operator=(bidirectional_iterator_proxy<OIt, OV, OPubV, _OItCat, _ODiff>&& other) {
        forward_iterator_proxy<It, V, PubV, _ItCat, _Diff>::operator=(std::move(other));
        return *this;
    }
    template<class T,
             class = std::enable_if_t<!is_bidirectional_iterator_proxy<std::decay_t<T>>::value, void>>
    bidirectional_iterator_proxy& operator=(T&& obj) {
        forward_iterator_proxy<It, V, PubV, _ItCat, _Diff>::operator=(std::forward<T>(obj));
        return *this;
    }

    bidirectional_iterator_proxy& operator++() {
        ++this->it_;
        return *this;
    }
    bidirectional_iterator_proxy operator++(int) {
        bidirectional_iterator_proxy oldit(*this);
        ++*this;
        return oldit;
    }

    bidirectional_iterator_proxy& operator--() {
        --this->it_;
        return *this;
    }
    bidirectional_iterator_proxy operator--(int) {
        bidirectional_iterator_proxy oldit(*this);
        --*this;
        return oldit;
    }
};

/**
 * @internal
 * @brief Random-access iterator that publishes a different reference type.
 * @headerfile lazy_sorted_container.h <coveo/lazy/detail/lazy_sorted_container.h>
 *
 * Random-access iterator proxy that wraps another random-access iterator but
 * converts the reference returned by <tt>operator*()</tt> to another reference type.
 *
 * @tparam It Type of iterator to proxy.
 * @tparam V Type of values returned by @c It.
 * @tparam PubV Type of values to publicly return references of.
 *              Must be either @c V or a base class of @c V.
 * @tparam _ItCat Iterator category for @c It.
 *                Uses <tt>std::iterator_traits</tt> by default.
 * @tparam _Diff Type used for differences between iterators.
 *               Uses <tt>std::iterator_traits</tt> by default.
 */
template<class It,
         class V,
         class PubV,
         class _ItCat = typename std::iterator_traits<It>::iterator_category,
         class _Diff = typename std::iterator_traits<It>::difference_type>
class random_access_iterator_proxy : public bidirectional_iterator_proxy<It, V, PubV, _ItCat, _Diff>
{
    static_assert(std::is_base_of<std::random_access_iterator_tag, _ItCat>::value,
                  "random_access_iterator_proxy can only wrap random-access iterators");

    // Helper used to identify our type
    template<class T> struct is_random_access_iterator_proxy : std::false_type { };
    template<class OIt, class OV, class OPubV, class _OItCat, class _ODiff>
    struct is_random_access_iterator_proxy<random_access_iterator_proxy<OIt, OV, OPubV, _OItCat, _ODiff>> : std::true_type { };
public:
    random_access_iterator_proxy() = default;
    random_access_iterator_proxy(const random_access_iterator_proxy& other) = default;
    random_access_iterator_proxy(random_access_iterator_proxy&& other) = default;
    template<class OIt, class OV, class OPubV, class _OItCat, class _ODiff>
    random_access_iterator_proxy(const random_access_iterator_proxy<OIt, OV, OPubV, _OItCat, _ODiff>& other)
        : bidirectional_iterator_proxy<It, V, PubV, _ItCat, _Diff>(static_cast<const bidirectional_iterator_proxy<OIt, OV, OPubV, _OItCat, _ODiff>&>(other)) { }
    template<class OIt, class OV, class OPubV, class _OItCat, class _ODiff>
    random_access_iterator_proxy(random_access_iterator_proxy<OIt, OV, OPubV, _OItCat, _ODiff>&& other)
        : bidirectional_iterator_proxy<It, V, PubV, _ItCat, _Diff>(static_cast<bidirectional_iterator_proxy<OIt, OV, OPubV, _OItCat, _ODiff>&&>(other)) { }
    template<class T,
             class = std::enable_if_t<!is_random_access_iterator_proxy<std::decay_t<T>>::value, void>>
    random_access_iterator_proxy(T&& obj)
        : bidirectional_iterator_proxy<It, V, PubV, _ItCat, _Diff>(std::forward<T>(obj)) { }
    template<class T1, class T2, class... Tx>
    random_access_iterator_proxy(T1&& obj1, T2&& obj2, Tx&&... objx)
        : bidirectional_iterator_proxy<It, V, PubV, _ItCat, _Diff>(std::forward<T1>(obj1), std::forward<T2>(obj2), std::forward<Tx>(objx)...) { }

    random_access_iterator_proxy& operator=(const random_access_iterator_proxy& other) = default;
    random_access_iterator_proxy& operator=(random_access_iterator_proxy&& other) = default;
    template<class OIt, class OV, class OPubV, class _OItCat, class _ODiff>
    random_access_iterator_proxy& operator=(const random_access_iterator_proxy<OIt, OV, OPubV, _OItCat, _ODiff>& other) {
        bidirectional_iterator_proxy<It, V, PubV, _ItCat, _Diff>::operator=(other);
        return *this;
    }
    template<class OIt, class OV, class OPubV, class _OItCat, class _ODiff>
    random_access_iterator_proxy& operator=(random_access_iterator_proxy<OIt, OV, OPubV, _OItCat, _ODiff>&& other) {
        bidirectional_iterator_proxy<It, V, PubV, _ItCat, _Diff>::operator=(std::move(other));
        return *this;
    }
    template<class T,
             class = std::enable_if_t<!is_random_access_iterator_proxy<std::decay_t<T>>::value, void>>
    random_access_iterator_proxy& operator=(T&& obj) {
        bidirectional_iterator_proxy<It, V, PubV, _ItCat, _Diff>::operator=(std::forward<T>(obj));
        return *this;
    }

    PubV& operator[](_Diff n) const { return this->it_[n]; }

    random_access_iterator_proxy& operator++() {
        ++this->it_;
        return *this;
    }
    random_access_iterator_proxy operator++(int) {
        random_access_iterator_proxy oldit(*this);
        ++*this;
        return oldit;
    }
    random_access_iterator_proxy& operator+=(_Diff n) {
        this->it_ += n;
        return *this;
    }
    friend random_access_iterator_proxy operator+(random_access_iterator_proxy it, _Diff n) {
        it += n;
        return it;
    }
    friend random_access_iterator_proxy operator+(_Diff n, random_access_iterator_proxy it) {
        it += n;
        return it;
    }

    random_access_iterator_proxy& operator--() {
        --this->it_;
        return *this;
    }
    random_access_iterator_proxy operator--(int) {
        random_access_iterator_proxy oldit(*this);
        --*this;
        return oldit;
    }
    random_access_iterator_proxy& operator-=(_Diff n) {
        this->it_ -= n;
        return *this;
    }
    friend random_access_iterator_proxy operator-(random_access_iterator_proxy it, _Diff n) {
        it -= n;
        return it;
    }
    decltype(auto) operator-(const random_access_iterator_proxy& right) const {
        return this->it_ - right.it_;
    }

    friend decltype(auto) operator<(const random_access_iterator_proxy& left, const random_access_iterator_proxy& right) {
        return left.it_ < right.it_;
    }
    friend decltype(auto) operator<=(const random_access_iterator_proxy& left, const random_access_iterator_proxy& right) {
        return left.it_ <= right.it_;
    }
    friend decltype(auto) operator>(const random_access_iterator_proxy& left, const random_access_iterator_proxy& right) {
        return left.it_ > right.it_;
    }
    friend decltype(auto) operator>=(const random_access_iterator_proxy& left, const random_access_iterator_proxy& right) {
        return left.it_ >= right.it_;
    }
};

/// @cond NEVERSHOWN

/**
 * @internal
 * @brief Generic iterator proxy.
 * @headerfile lazy_sorted_container.h <coveo/lazy/detail/lazy_sorted_container.h>
 *
 * Iterator proxy that wraps another iterator but converts the reference returned
 * by <tt>operator*()</tt> to another reference type. Defined as follows:
 *
 * - If @c It is a pointer type, then @c iterator_proxy is <tt>PubV*</tt>.
 * - Otherwise, if @c It is a random-access iterator, then @c iterator_proxy is @c random_access_iterator_proxy.
 * - Otherwise, if @c It is a bidirectional iterator, then @c iterator_proxy is @c bidirectional_iterator_proxy.
 * - Otherwise, if @c It is a forward iterator, then @c iterator_proxy is @c forward_iterator_proxy.
 * - Otherwise, @c iterator_proxy is an incomplete type.
 *
 * @tparam It Type of iterator to proxy.
 * @tparam V Type of values returned by @c It.
 * @tparam PubV Type of values to publicly return references of.
 *              Must be either @c V or a base class of @c V.
 * @tparam _ItCat Iterator category for @c It.
 *                Uses <tt>std::iterator_traits</tt> by default.
 * @tparam _Diff Type used for differences between iterators.
 *               Uses <tt>std::iterator_traits</tt> by default.
 */
template<class It,
         class V,
         class PubV,
         class _ItCat = typename std::iterator_traits<It>::iterator_category,
         class _Diff = typename std::iterator_traits<It>::difference_type>
using iterator_proxy = std::conditional_t<std::is_pointer<It>::value,
                                          PubV*,
                            std::conditional_t<std::is_base_of<std::random_access_iterator_tag, _ItCat>::value,
                                               random_access_iterator_proxy<It, V, PubV, _ItCat, _Diff>,
                                std::conditional_t<std::is_base_of<std::bidirectional_iterator_tag, _ItCat>::value,
                                                   bidirectional_iterator_proxy<It, V, PubV, _ItCat, _Diff>,
                                    std::conditional_t<std::is_base_of<std::forward_iterator_tag, _ItCat>::value,
                                                       forward_iterator_proxy<It, V, PubV, _ItCat, _Diff>,
                                                       incomplete_type>>>>;

/**
 * @internal
 * @brief Generic conditional iterator proxy.
 * @headerfile lazy_sorted_container.h <coveo/lazy/detail/lazy_sorted_container.h>
 *
 * Iterator proxy that can wraps another iterator but convert the reference
 * returned by <tt>operator*()</tt> to another reference type if needed. Defined as follows:
 *
 * - If @c PubV is the same type as @c V, then @c conditional_iterator_proxy is @c It.
 * - Otherwise, @c conditional_iterator_proxy is @c iterator_proxy.
 *
 * @tparam It Type of iterator to proxy.
 * @tparam V Type of values returned by @c It.
 * @tparam PubV Type of values to publicly return references of.
 *              Must be either @c V or a base class of @c V.
 * @tparam _ItCat Iterator category for @c It.
 *                Uses <tt>std::iterator_traits</tt> by default.
 * @tparam _Diff Type used for differences between iterators.
 *               Uses <tt>std::iterator_traits</tt> by default.
 */
template<class It,
         class V,
         class PubV,
         class _ItCat = typename std::iterator_traits<It>::iterator_category,
         class _Diff = typename std::iterator_traits<It>::difference_type>
using conditional_iterator_proxy = std::conditional_t<std::is_same<PubV, V>::value,
                                                      It,
                                                      iterator_proxy<It, V, PubV, _ItCat, _Diff>>;

/// @endcond

/**
 * @internal
 * @brief Helper for @c sort_if_needed.
 * @headerfile lazy_sorted_container.h <coveo/lazy/detail/lazy_sorted_container.h>
 *
 * Helper type that calls <tt>sort_if_needed()</tt> on a lazy sorted container
 * only if it does not support duplicates.
 *
 * @tparam Multi Whether lazy sorted container accepts duplicates.
 */
template<bool Multi> struct sort_lazy_container_if_needed_and_not_multi;
template<> struct sort_lazy_container_if_needed_and_not_multi<true> {
    template<class LazyC> void operator()(LazyC&) const { }
};
template<> struct sort_lazy_container_if_needed_and_not_multi<false> {
    template<class LazyC> void operator()(LazyC& c) const { c.sort_if_needed(); }
};

/**
 * @internal
 * @brief Sort helper for lazy sorted containers.
 * @headerfile lazy_sorted_container.h <coveo/lazy/detail/lazy_sorted_container.h>
 *
 * Helper type that sorts the elements of a lazy sorted container.
 * Implementation differs depending on whether containers accepts duplicates or not.
 *
 * @tparam Multi Whether lazy sorted container accepts duplicates.
 */
template<bool Multi> struct sort_lazy_container_elements;
template<> struct sort_lazy_container_elements<true> {
    template<class LazyC> void operator()(LazyC& c) const {
        // For multi-value containers, we need to use std::stable_sort() because order
        // of equivalent elements must be preserved (since C++11).
        std::stable_sort(c.elements_.begin(), c.elements_.end(), c.vcmp_);
    }
};
template<> struct sort_lazy_container_elements<false> {
    template<class LazyC> void operator()(LazyC& c) const {
        std::sort(c.elements_.begin(), c.elements_.end(), c.vcmp_);
        c.elements_.erase(std::unique(c.elements_.begin(), c.elements_.end(), c.veq_), c.elements_.end());
    }
};

/**
 * @internal
 * @brief Helper that updates a lazy sorted container after insertion.
 * @headerfile lazy_sorted_container.h <coveo/lazy/detail/lazy_sorted_container.h>
 *
 * Helper type that updates a lazy sorted container's @c sorted_ flag after an
 * insertion at the back of the container, depending on whether the insertion
 * maintained the sorting or not. Use only on sorted containers with more
 * than one element.
 *
 * @tparam Multi Whether lazy sorted container accepts duplicates.
 */
template<bool Multi> struct updated_lazy_container_sorted_flag_after_insert;
template<> struct updated_lazy_container_sorted_flag_after_insert<true> {
    template<class LazyC> void operator()(LazyC& c) const {
        c.sorted_ = !c.vcmp_(c.elements_.back(), *(c.elements_.crbegin() + 1));
    }
};
template<> struct updated_lazy_container_sorted_flag_after_insert<false> {
    template<class LazyC> void operator()(LazyC& c) const {
        c.sorted_ = c.vcmp_(*(c.elements_.crbegin() + 1), c.elements_.back());
    }
};

/**
 * @brief Base class for lazy-sorted containers.
 * @headerfile lazy_sorted_container.h <coveo/lazy/detail/lazy_sorted_container.h>
 *
 * Base type of @c lazy_sorted_container that includes a typedef @c mapped_type if
 * @c T is not @c void, otherwise no typedef. Used to include the typedef for maps only.
 *
 * @tparam T Type of values bound to the container's keys, or @c void if
 *           the container is not a map.
 */
template<class T> struct mapped_type_base {
    /**
     * @brief Type of values stored in the map. Absent for set-like containers.
     */
    using mapped_type = T;
};
template<> struct mapped_type_base<void> { };

/**
 * @brief Template base class for lazy-sorted associative containers.
 * @headerfile lazy_sorted_container.h <coveo/lazy/detail/lazy_sorted_container.h>
 *
 * Implementation of a container that maintains a sorted set of elements, like
 * <tt>std::set</tt> or <tt>std::map</tt>. The container uses another container
 * internally to store elements in order to optimize size and speed; the container
 * is sorted only when needed (or it can be triggered on-demand).
 *
 * This class meets the requirements of @c Container, @c ReversibleContainer,
 * @c AllocatorAwareContainer and @c AssociativeContainer. Furthermore, the internal
 * container type must meet the requirements of @c Container, @c ReversibleContainer,
 * @c AllocatorAwareContainer and @c SequenceContainer. For details on those concepts,
 * see http://en.cppreference.com/w/cpp/concept
 *
 * This class is not thread-safe; furthermore, because it performs on-demand
 * sorting, there are only two ways of using it in a multi-threaded context:
 * - Protect it with an exclusive mutex like <tt>std::mutex</tt>, or
 * - Protect it with a shared mutex like <tt>std::shared_mutex</tt> and make sure
 *   that it is always sorted before writer threads release their write locks.
 *
 * If this container is a map (e.g., type @c T is not @c void), this class will
 * additionally have a @c mapped_type typedef (defined as @c T). Furthermore, if
 * the map does not accept duplicates (e.g., @c Multi is @c false), the following
 * additional methods are included:
 *
 * - <tt>at()</tt>
 * - <tt>operator[]()</tt>
 * - <tt>insert_or_assign()</tt>
 * - <tt>try_emplace()</tt>
 *
 * Note, however, that those methods all require the container to be sorted to
 * work, so they could be less efficient that blind inserts.
 *
 * @tparam K Type of keys used to sort elements in the container.
 * @tparam T Type of values bound to each key. If this is set to @c void, the
 *           container is assumed to be a set (like <tt>std::set</tt>) and its iterators
 *           will always return const references. Otherwise, this is assumed to be
 *           a map (like <tt>std::map</tt>) and will contain extra elements (see above).
 * @tparam V Type of elements stored in the container.
 * @tparam PubV Type of elements that container should appear to contain. Iterators
 *              will return references to this type. Must either be the same as @c V
 *              or a derived type (e.g., <tt>std::is_base_of<PubV, V>::value</tt> is
 *              @c true).
 * @tparam VToK Predicate that, given a <tt>const V&</tt>, will return the key to use
 *              to sort the element. For best performance, this should be a
 *              <tt>const K&</tt> pointing to a key inside the element.
 * @tparam KCmp Predicate used to compare keys.
 * @tparam KEq Predicate used to test keys for equality.
 * @tparam Alloc Type of allocator to use for the underlying storage.
 * @tparam Impl Type of container to use for internal storage. Must have the same
 *              template parameters as <tt>std::vector/deque/etc.</tt>
 * @tparam Multi Whether container is allowed to have duplicates. Set this to
 *               @c true for <tt>std::multiset/multimap</tt>-like containers.
 */
template<class K,
         class T,
         class V,
         class PubV,
         class VToK,
         class KCmp,
         class KEq,
         class Alloc,
         template<class _ImplT, class _ImplAlloc> class Impl,
         bool Multi,
         bool _IsNonMultiMap = !std::is_void<T>::value && !Multi>
class lazy_sorted_container : public mapped_type_base<T>
{
public:
    /**
     * @brief Type of container used internally.
     *
     * Type of container used internally to store the elements. Usually a contiguous container offering
     * efficient insertion at the back. Defaults to <tt>std::vector</tt>.
     */
    using container_impl = Impl<V, Alloc>;

    /**
     * @brief Internal container's @c iterator implementation.
     *
     * Implementation of @c iterator by the internal container. Not used directly,
     * but conditionally to define <tt>lazy_sorted_container::iterator</tt>.
     */
    using iterator_impl = typename container_impl::iterator;

    /**
     * @brief Internal container's @c const_iterator implementation.
     *
     * Implementation of @c const_iterator by the internal container. Not used directly,
     * but conditionally to define <tt>lazy_sorted_container::const_iterator</tt>.
     */
    using const_iterator_impl = typename container_impl::const_iterator;

    /**
     * @brief Internal container's @c reverse_iterator implementation.
     *
     * Implementation of @c reverse_iterator by the internal container. Not used directly,
     * but conditionally to define <tt>lazy_sorted_container::reverse_iterator</tt>.
     */
    using reverse_iterator_impl = typename container_impl::reverse_iterator;

    /**
     * @brief Internal container's @c const_reverse_iterator implementation.
     *
     * Implementation of @c const_reverse_iterator by the internal container.
     * Not used directly, but conditionally to define <tt>lazy_sorted_container::const_reverse_iterator</tt>.
     */
    using const_reverse_iterator_impl = typename container_impl::const_reverse_iterator;
    
    /**
     * @brief Type of keys used by the container.
     *
     * Type of keys used in the container. For sets, this is the same as <tt>lazy_sorted_container::value_type</tt>;
     * for maps, this is a different type. Keys are used to perform lookups in the container
     * (e.g. <tt>find()</tt>, <tt>lower_bound()</tt>, etc.)
     */
    using key_type = K;

    /**
     * @brief Type of elements stored in the container.
     *
     * Type of the elements, or "values", stored in the container. For sets, this is the
     * same type as <tt>lazy_sorted_container::key_type</tt>; for maps, this corresponds
     * to a key/value pair.
     */
    using value_type = PubV;

    /**
     * @brief Type used to represent size of container.
     *
     * Type used to represent the size of the container, e.g. the number of elements it contains.
     * Corresponds to the type used by the internal container (usually <tt>std::size_t</tt>).
     */
    using size_type = typename container_impl::size_type;

    /**
     * @brief Type used to represent difference in the container.
     *
     * Type used to represent the difference between two positions in the container.
     * Corresponds to the type used by the internal container (usually <tt>std::ptrdiff_t</tt>).
     */
    using difference_type = typename container_impl::difference_type;

    /**
     * @brief Predicate to get keys from values.
     *
     * Predicate that, given a container element or "value", can extract its key. This is used
     * internally to keep values sorted. For sets, this predicate simply returns the value itself;
     * for maps, it returns the pair's @c first member.
     */
    using value_to_key = VToK;

    /**
     * @brief Predicate to compare keys.
     *
     * Predicate that can compare keys and tell if one is "less" (e.g. goes before) the other.
     * This predicate must impose a strict ordering of the keys. Used internally to keep values
     * sorted and to perform lookups.
     */
    using key_compare = KCmp;

    /**
     * @brief Predicate to compare values.
     *
     * Same as <tt>lazy_sorted_container::key_compare</tt>, but compares values directly
     * by using <tt>lazy_sorted_container::value_to_key</tt> to extract a key for each
     * value compared.
     */
    using value_compare = lazy_value_pred_proxy<value_type, value_to_key, key_compare>;

    /**
     * @brief Predicate to check keys for equality.
     *
     * Predicate that can compare keys and if one is equal to the other. Used internally
     * to identify duplicates if needed (e.g., not a @c multimap or @c multiset).
     */
    using key_equal_to = KEq;

    /**
     * @brief Predicate to check values for equality.
     *
     * Same as <tt>lazy_sorted_container::key_equal_to</tt>, but compares values directly
     * by using <tt>lazy_sorted_container::value_to_key</tt> to extract a key for each
     * value compared.
     */
    using value_equal_to = lazy_value_pred_proxy<value_type, value_to_key, key_equal_to>;

    /**
     * @brief Type of allocator used.
     *
     * Type of allocator used to allocate memory in the container. Defaults to the
     * allocator used by the internal container, which is usually <tt>std::allocator</tt>
     * in the end.
     */
    using allocator_type = typename container_impl::allocator_type;

    /**
     * @brief Type of reference returned by <tt>iterator</tt>s.
     *
     * Type of reference returned by the container's <tt>iterator</tt>s. For maps, this is
     * a non-const reference that allows the caller to modify an element; for sets,
     * this is a const reference and this is similar to <tt>lazy_sorted_container::const_reference</tt>.
     */
    using reference = std::conditional_t<std::is_void<T>::value, const value_type&, value_type&>;

    /**
     * @brief Type of reference returned by <tt>const_iterator</tt>s.
     *
     * Type of reference returned by the container's <tt>const_iterator</tt>s. Always a
     * const reference regardless of container type.
     */
    using const_reference = const value_type&;

    /**
     * @brief Type of pointer corresponding to @c reference.
     *
     * Type of pointer corresponding to <tt>lazy_sorted_container::reference</tt>. For map, this
     * points to a non-const value that can be modified; for sets, this points to a const value
     * and this is similar to <tt>lazy_sorted_container::const_pointer</tt>.
     */
    using pointer = std::conditional_t<std::is_void<T>::value, const value_type*, value_type*>;

    /**
     * @brief Type of pointer corresponding to @c const_reference.
     *
     * Type of pointer corresponding to <tt>lazy_sorted_container::const_reference</tt>.
     * Always points to a const value regardless of container type.
     */
    using const_pointer = const value_type*;

    /**
     * @brief Iterator for the container.
     *
     * Iterator type for this container. For maps, returns non-const references and allows
     * elements to be modified; for sets, this is similar to <tt>lazy_sorted_container::const_iterator</tt>.
     */
    using iterator = std::conditional_t<std::is_void<T>::value,
                                        conditional_iterator_proxy<const_iterator_impl, const V, const PubV>,
                                        conditional_iterator_proxy<iterator_impl, V, PubV>>;

    /**
     * @brief Const iterator for the container.
     *
     * Const iterator type for this container. Always returns const references to the elements
     * regardless of container type.
     */
    using const_iterator = conditional_iterator_proxy<const_iterator_impl, const V, const PubV>;

    /**
     * @brief Reverse iterator for the container.
     *
     * Iterator type for this container allowing reverse iteration. For maps, returns non-const
     * references and allows elements to be modified; for sets, this is similar to
     * <tt>lazy_sorted_container::const_reverse_iterator</tt>.
     */
    using reverse_iterator = std::conditional_t<std::is_void<T>::value,
                                                conditional_iterator_proxy<const_reverse_iterator_impl, const V, const PubV>,
                                                conditional_iterator_proxy<reverse_iterator_impl, V, PubV>>;

    /**
     * @brief Const reverse iterator for the container.
     *
     * Const iterator type for this container allowing reverse iteration. Always returns
     * const references to the elements regardless of container type.
     */
    using const_reverse_iterator = conditional_iterator_proxy<const_reverse_iterator_impl, const V, const PubV>;

private:
    mutable container_impl elements_;   // Container storing actual elements.
    mutable bool sorted_;               // Whether elements_ is currently sorted.
    value_to_key vtok_;                 // Predicate to get key for a given value.
    value_compare vcmp_;                // Value comparator.
    value_equal_to veq_;                // Value equality predictate.

    /// @cond NEVERSHOWN

private:
    // Friend some helper types.
    template<bool _HelperMulti> friend struct sort_lazy_container_if_needed_and_not_multi;
    template<bool _HelperMulti> friend struct sort_lazy_container_elements;
    template<bool _HelperMulti> friend struct updated_lazy_container_sorted_flag_after_insert;
    
    /// @endcond

public:
    /**
     * @brief Default constructor.
     *
     * Default constructor. Creates an empty container with default predicates.
     */
    lazy_sorted_container()
        : lazy_sorted_container(key_compare()) { }

    /**
     * @brief Constructor with key comparator.
     *
     * Constructor with a specific key comparator instance.
     *
     * @param kcmp @c key_compare instance to use for this container.
     * @param alloc @c allocator_type instance to use for this container. Defaults to
     *              a default-constructed <tt>lazy_sorted_container::allocator_type</tt>.
     * @param keq @c key_equal_to instance to use for this container. Defaults to
     *            a default-constructed <tt>lazy_sorted_container::key_equal_to</tt>.
     */
    explicit lazy_sorted_container(const key_compare& kcmp,
                                   const allocator_type& alloc = allocator_type(),
                                   const key_equal_to& keq = key_equal_to())
        : elements_(alloc), sorted_(true),
          vtok_(), vcmp_(vtok_, kcmp),  veq_(vtok_, keq) { }

    /**
     * @brief Constructor with allocator.
     *
     * Constructor with a specific allocator instance.
     *
     * @param alloc @c allocator_type instance to use for this container.
     */
    explicit lazy_sorted_container(const allocator_type& alloc)
        : lazy_sorted_container(key_compare(), alloc) { }

    /**
     * @brief Range constructor.
     *
     * Constructor that initializes the container with the elements in the
     * range <tt>[first, last[</tt>.
     *
     * @param first Beginning of the range of elements to insert in the container (inclusive).
     * @param last End of the range of elements to insert in the container (exclusive).
     * @param kcmp @c key_compare instance to use for this container. Defaults to
     *             a default-constructed <tt>lazy_sorted_container::key_compare</tt>.
     * @param alloc @c allocator_type instance to use for this container. Defaults to
     *              a default-constructed <tt>lazy_sorted_container::allocator_type</tt>.
     * @param keq @c key_equal_to instance to use for this container. Defaults to
     *            a default-constructed <tt>lazy_sorted_container::key_equal_to</tt>.
     */
    template<class It> lazy_sorted_container(It first,
                                             It last,
                                             const key_compare& kcmp = key_compare(),
                                             const allocator_type& alloc = allocator_type(),
                                             const key_equal_to& keq = key_equal_to())
        : elements_(first, last, alloc), sorted_(elements_.size() <= 1),
          vtok_(), vcmp_(vtok_, kcmp), veq_(vtok_, keq) { }

    /**
     * @brief Range constructor with allocator.
     *
     * Constructor that initializes the container with the elements in the
     * range <tt>[first, last[</tt>. Also initializes the allocator.
     *
     * @param first Beginning of the range of elements to insert in the container (inclusive).
     * @param last End of the range of elements to insert in the container (exclusive).
     * @param alloc @c allocator_type instance to use for this container.
     */
    template<class It> lazy_sorted_container(It first,
                                             It last,
                                             const allocator_type& alloc)
        : lazy_sorted_container(first, last, key_compare(), alloc) { }

    /**
     * @brief Copy constructor.
     *
     * Copy constructor. Copies the elements of another container of the same type.
     *
     * @param obj Container to copy.
     */
    lazy_sorted_container(const lazy_sorted_container& obj) = default;

    /**
     * @brief Copy constructor with allocator.
     *
     * Constructor that copies the elements of another container but uses a
     * different allocator instance.
     *
     * @param obj Container to copy. Everything is copied except its allocator.
     * @param alloc @c allocator_type instance  to use for this container.
     */
    lazy_sorted_container(const lazy_sorted_container& obj, const allocator_type& alloc)
        : elements_(obj.elements_, alloc), sorted_(obj.sorted_),
          vtok_(obj.vtok_), vcmp_(obj.vcmp_), veq_(obj.veq_) { }

    /**
     * @brief Move constructor.
     *
     * Move constructor. Moves the elements of another container into this one; no copy is performed.
     *
     * @param obj Container to move in this one.
     */
    lazy_sorted_container(lazy_sorted_container&& obj)
        : elements_(std::move(obj.elements_)), sorted_(obj.sorted_),
          vtok_(std::move(obj.vtok_)), vcmp_(std::move(obj.vcmp_)), veq_(std::move(obj.veq_)) {
        obj.sorted_ = true;
    }

    /**
     * @brief Move constructor with allocator.
     *
     * Constructor that moves the elements of another container in this one
     * but uses a different allocator instance.
     *
     * @param obj Container to move in this one. Its allocator is not moved.
     * @param alloc @c allocator_type instance to use for this container.
     */
    lazy_sorted_container(lazy_sorted_container&& obj, const allocator_type& alloc)
        : elements_(std::move(obj.elements_), alloc), sorted_(obj.sorted_),
          vtok_(std::move(obj.vtok_)), vcmp_(std::move(obj.vcmp_)), veq_(std::move(obj.veq_)) {
        obj.sorted_ = true;
    }

    /**
     * @brief Initializer list constructor.
     *
     * Constructor that initializes the container with the elements in the
     * given @c initializer_list.
     *
     * @param init @c initializer_list containing container's initial elements.
     * @param kcmp @c key_compare instance to use for this container. Defaults to
     *             a default-constructed <tt>lazy_sorted_container::key_compare</tt>.
     * @param alloc @c allocator_type instance to use for this container. Defaults to
     *              a default-constructed <tt>lazy_sorted_container::allocator_type</tt>.
     * @param keq @c key_equal_to instance to use for this container. Defaults to
     *            a default-constructed <tt>lazy_sorted_container::key_equal_to</tt>.
     */
    lazy_sorted_container(std::initializer_list<value_type> init,
                          const key_compare& kcmp = key_compare(),
                          const allocator_type& alloc = allocator_type(),
                          const key_equal_to& keq = key_equal_to())
        : lazy_sorted_container(std::begin(init), std::end(init), kcmp, alloc, keq) { }

    /**
     * @brief Initializer list constructor with allocator.
     *
     * Constructor that initializes the container with the elements in the
     * given @c initializer_list. Also specifies an allocator instance.
     *
     * @param init @c initializer_list containing container's initial elements.
     * @param alloc @c allocator_type instance to use for this container.
     */
    lazy_sorted_container(std::initializer_list<value_type> init,
                          const allocator_type& alloc)
        : lazy_sorted_container(std::begin(init), std::end(init), alloc) { }

    /**
     * @brief Assignment operator.
     *
     * Assignment operator. Copies the elements of the given container.
     *
     * @param obj Container to copy.
     * @return Reference to @c this container.
     */
    lazy_sorted_container& operator=(const lazy_sorted_container& obj) = default;

    /**
     * @brief Move assignment operator.
     *
     * Move assignment operator. Moves the elements of the given container into
     * this one; no copy is performed.
     *
     * @param obj Container to move in this one.
     * @return Reference to @c this container.
     */
    lazy_sorted_container& operator=(lazy_sorted_container&& obj) {
        elements_ = std::move(obj.elements_);
        sorted_ = obj.sorted_;
        vtok_ = std::move(obj.vtok_);
        vcmp_ = std::move(obj.vcmp_);
        veq_ = std::move(obj.veq_);
        obj.sorted_ = true;
        return *this;
    }

    /**
     * @brief Assignment operator from initializer list.
     *
     * Assignment operator that copies the elements in the given @c initializer_list
     * into this container, replacing its content.
     *
     * @param init @c initializer_list containing the elements to copy in this container.
     * @return Reference to @c this container.
     */
    lazy_sorted_container& operator=(std::initializer_list<value_type> init) {
        elements_.clear();
        elements_.insert(elements_.cend(), std::begin(init), std::end(init));
        sorted_ = elements_.size() <= 1;
        return *this;
    }

    /**
     * @brief Equality operator.
     *
     * Operator that verifies if two containers contain the same elements. Note: does not use
     * @c key_equal_to to compare the elements; elements must be @c EqualityComparable for this
     * to work. For more info, see http://en.cppreference.com/w/cpp/concept/EqualityComparable
     *
     * @param left First container to compare.
     * @param right Second container to compare.
     * @return @c true if @c left contains the same elements as @c right.
     */
    friend bool operator==(const lazy_sorted_container& left, const lazy_sorted_container& right) {
        // We need both containers to be sorted for this to work.
        left.sort_if_needed();
        right.sort_if_needed();
        return std::equal(left.elements_.cbegin(), left.elements_.cend(),
                          right.elements_.cbegin(), right.elements_.cend());
    }
    
    /**
     * @brief Inequality operator.
     *
     * Operator that verifies if two containers have different elements.
     * For more info, see <tt>operator==()</tt>.
     *
     * @param left First container to compare.
     * @param right Second container to compare.
     * @return @c true if @c left does not contain the same elements as @c right.
     */
    friend bool operator!=(const lazy_sorted_container& left, const lazy_sorted_container& right) {
        return !(left == right);
    }

    /**
     * @brief "Less than" comparison operator.
     *
     * Operator that verifies if a container is "less than" another by performing a lexicographical
     * comparison of the elements. Note: does not use @c key_compare to compare the elements;
     * elements must be @c LessThanComparable for this to work. For more info, see
     * http://en.cppreference.com/w/cpp/concept/LessThanComparable
     *
     * @param left First container to compare.
     * @param right Second container to compare.
     * @return @c true if @c left is "less than" @c right.
     */
    friend bool operator<(const lazy_sorted_container& left, const lazy_sorted_container& right) {
        // We need both containers to be sorted for this to work.
        left.sort_if_needed();
        right.sort_if_needed();
        return std::lexicographical_compare(left.elements_.cbegin(), left.elements_.cend(),
                                            right.elements_.cbegin(), right.elements_.cend());
    }

    /**
     * @brief "Less than or equal to" comparison operator.
     *
     * Operator that verifies if a container is "less than or equal to" another.
     * For more information, see <tt>operator<()</tt>.
     *
     * @param left First container to compare.
     * @param right Second container to compare.
     * @return @c true if @left is "less than or equal to" @c right.
     */
    friend bool operator<=(const lazy_sorted_container& left, const lazy_sorted_container& right) {
        return !(right < left);
    }

    /**
     * @brief "Greater than" comparison operator.
     *
     * Operator that verifies if a container is "greater than" another.
     * For more information, see <tt>operator<()</tt>.
     *
     * @param left First container to compare.
     * @param right Second container to compare.
     * @return @c true if @left is "greater than" @c right.
     */
    friend bool operator>(const lazy_sorted_container& left, const lazy_sorted_container& right) {
        return right < left;
    }

    /**
     * @brief "Greater than or equal to" comparison operator.
     *
     * Operator that verifies if a container is "greater than or equal to" another.
     * For more information, see <tt>operator<()</tt>.
     *
     * @param left First container to compare.
     * @param right Second container to compare.
     * @return @c true if @left is "greater than or equal to" @c right.
     */
    friend bool operator>=(const lazy_sorted_container& left, const lazy_sorted_container& right) {
        return !(left < right);
    }

    /**
     * @brief Accesses an existing element.
     *
     * Returns a reference to the existing element with the given key.
     * If the container does not have such an element, an exception is thrown.
     *
     * @param key Key of element to look for.
     * @return Reference to the existing element associated with @c key.
     * @throw coveo::lazy::out_of_range No element with that key exists.
     * @remarks This method is only available for non-multi maps.
     */
    template<class _T = T,
             class _TRef = std::enable_if_t<_IsNonMultiMap, _T&>>
    _TRef at(const key_type& key) {
        sort_if_needed();
        auto elem_end = elements_.end();
        auto it = std::lower_bound(elements_.begin(), elem_end, key, vcmp_);
        if (it == elem_end || vcmp_(key, *it)) {
            throw_out_of_range();
        }
        return it->second;
    }

    /**
     * @brief Accesses an existing element (const version).
     *
     * Returns a const reference to the existing element with the given key.
     * If the container does not have such an element, an exception is thrown.
     *
     * @param key Key of element to look for.
     * @return Const reference to the existing element associated with @c key.
     * @throw coveo::lazy::out_of_range No element with that key exists.
     * @remarks This method is only available for non-multi maps.
     */
    template<class _T = T,
             class _CTRef = std::enable_if_t<_IsNonMultiMap, const _T&>>
    _CTRef at(const key_type& key) const {
        sort_if_needed();
        auto elem_cend = elements_.cend();
        auto cit = std::lower_bound(elements_.cbegin(), elem_cend, key, vcmp_);
        if (cit == elem_cend || vcmp_(key, *cit)) {
            throw_out_of_range();
        }
        return cit->second;
    }

    /**
     * @brief Accesses an element, possibly creating it.
     *
     * Returns a reference to the element with the given key. If the
     * container does not have such an element, a default-constructed
     * one is added and its reference is returned.
     *
     * @param key Key of element to look for.
     * @return Reference to the element associated with @c key.
     * @remarks This method is only available for non-multi maps.
     */
    template<class _T = T,
             class _TRef = std::enable_if_t<_IsNonMultiMap, _T&>>
    _TRef operator[](const key_type& key) {
        return operator_brackets_impl(key);
    }

    /**
     * @brief Accesses an element, possibly creating it (move version).
     *
     * Returns a reference to the element with the given key. If the
     * container does not have such an element, a default-constructed
     * one is added and associated with a key that is move-constructed
     * from @c key.
     *
     * @param key Key of element to look for. If the element does not
     *            exist, the key is moved to the container and associated
     *            with the new element.
     * @return Reference to the element associated with @c key.
     * @remarks This method is only available for non-multi maps.
     */
    template<class _T = T,
             class _TRef = std::enable_if_t<_IsNonMultiMap, _T&>>
    _TRef operator[](key_type&& key) {
        return operator_brackets_impl(std::move(key));
    }

    /**
     * @brief Iterator to beginning of container.
     *
     * Returns an <tt>lazy_sorted_container::iterator</tt> that points to the
     * beginning of this container. Together with <tt>end()</tt>,
     * it can be used to enumerate the container's elements.
     *
     * The iterator allows elements to be modified if this container is a map.
     *
     * @return @c iterator to beginning of container.
     * @see lazy_sorted_container::end
     */
    iterator begin() {
        // Container always needs to be sorted when we iterate.
        sort_if_needed();
        // Note that this works even if iterator type is a proxy, because it has
        // an implicit converting constructor from the proxied iterator type.
        return elements_.begin();
    }

    /**
     * @brief Iterator to beginning of container (const version).
     *
     * Returns a <tt>lazy_sorted_container::const_iterator</tt> that points to
     * the beginning of this container. Together with <tt>end()</tt>,
     * it can be used to enumerate the container's elements.
     *
     * The iterator does not allow elements to be modified.
     *
     * @return @c const_iterator to beginning of container.
     * @see lazy_sorted_container::end
     */
    const_iterator begin() const {
        return cbegin();
    }

    /**
     * @brief Iterator to beginning of container (const version).
     *
     * Returns a <tt>lazy_sorted_container::const_iterator</tt> that points to
     * the beginning of this container. Together with <tt>cend()</tt>,
     * it can be used to enumerate the container's elements.
     *
     * The iterator does not allow elements to be modified.
     *
     * @return @c const_iterator to beginning of container.
     * @remark This method can be used to get a @c const_iterator for a
     *         container even if the container itself is not @c const.
     * @see lazy_sorted_container::cend
     */
    const_iterator cbegin() const {
        sort_if_needed();
        return elements_.cbegin();
    }

    /**
     * @brief Iterator to end of container.
     *
     * Returns an <tt>lazy_sorted_container::iterator</tt> that points to
     * the end of this container. Together with <tt>begin()</tt>,
     * it can be used to enumerate the container's elements.
     *
     * The iterator allows elements to be modified if this container is a map.
     *
     * @return @c iterator to end of container.
     * @see lazy_sorted_container::begin
     */
    iterator end() {
        sort_if_needed();
        return elements_.end();
    }

    /**
     * @brief Iterator to end of container (const version).
     *
     * Returns a <tt>lazy_sorted_container::const_iterator</tt> that points to
     * the end of this container. Together with <tt>begin()</tt>,
     * it can be used to enumerate the container's elements.
     *
     * The iterator does not allow elements to be modified.
     *
     * @return @c const_iterator to end of container.
     * @see lazy_sorted_container::begin
     */
    const_iterator end() const {
        return cend();
    }

    /**
     * @brief Iterator to end of container (const version).
     *
     * Returns a <tt>lazy_sorted_container::const_iterator</tt> that points to
     * the end of this container. Together with <tt>cbegin()</tt>,
     * it can be used to enumerate the container's elements.
     *
     * The iterator does not allow elements to be modified.
     *
     * @return @c const_iterator to end of container.
     * @remark This method can be used to get a @c const_iterator for a
     *         container even if the container itself is not @c const.
     * @see lazy_sorted_container::cbegin
     */
    const_iterator cend() const {
        sort_if_needed();
        return elements_.cend();
    }

    /**
     * @brief Iterator to beginning of container (in reverse).
     *
     * Returns a <tt>lazy_sorted_container::reverse_iterator</tt> that points to the
     * beginning of a reverse view of this container. Together with <tt>rend()</tt>,
     * it can be used to enumerate the container's elements, but in reverse.
     *
     * The iterator allows elements to be modified if this container is a map.
     *
     * @return @c reverse_iterator to beginning of container's reverse view.
     * @see lazy_sorted_container::rend
     */
    reverse_iterator rbegin() {
        sort_if_needed();
        return elements_.rbegin();
    }

    /**
     * @brief Iterator to beginning of container (in reverse - const version).
     *
     * Returns a <tt>lazy_sorted_container::const_reverse_iterator</tt> that
     * points to the beginning of a reverse view of this container. Together with
     * <tt>rend()</tt>, it can be used to enumerate the container's elements,
     * but in reverse.
     *
     * The iterator does not allow elements to be modified.
     *
     * @return @c const_reverse_iterator to beginning of container's reverse view.
     * @see lazy_sorted_container::rend
     */
    const_reverse_iterator rbegin() const {
        return crbegin();
    }

    /**
     * @brief Iterator to beginning of container (in reverse - const version).
     *
     * Returns a <tt>lazy_sorted_container::const_reverse_iterator</tt> that
     * points to the beginning of a reverse view of this container. Together with
     * <tt>crend()</tt>, it can be used to enumerate the container's elements,
     * but in reverse.
     *
     * The iterator does not allow elements to be modified.
     *
     * @return @c const_reverse_iterator to beginning of container's reverse view.
     * @remark This method can be used to get a @c const_reverse_iterator for a
     *         container even if the container itself is not @c const.
     * @see lazy_sorted_container::crend
     */
    const_reverse_iterator crbegin() const {
        sort_if_needed();
        return elements_.crbegin();
    }

    /**
     * @brief Iterator to end of container (in reverse).
     *
     * Returns a <tt>lazy_sorted_container::reverse_iterator</tt> that points to
     * the end of a reverse view of this container. Together with
     * <tt>rbegin()</tt>, it can be used to enumerate the container's
     * elements, but in reverse.
     *
     * The iterator allows elements to be modified if this container is a map.
     *
     * @return @c reverse_iterator to end of container.
     * @see lazy_sorted_container::rbegin
     */
    reverse_iterator rend() {
        sort_if_needed();
        return elements_.rend();
    }

    /**
     * @brief Iterator to end of container (in reverse - const version).
     *
     * Returns a <tt>lazy_sorted_container::const_reverse_iterator</tt> that
     * points to the end of a reverse view of this container. Together with
     * <tt>rbegin()</tt>, it can be used to enumerate the container's elements,
     * but in reverse.
     *
     * The iterator does not allow elements to be modified.
     *
     * @return @c const_reverse_iterator to end of container.
     * @see lazy_sorted_container::rbegin
     */
    const_reverse_iterator rend() const {
        return crend();
    }

    /**
     * @brief Iterator to end of container (in reverse - const version).
     *
     * Returns a <tt>lazy_sorted_container::const_reverse_iterator</tt> that
     * points to the end of a reverse view of this container. Together with
     * <tt>crbegin()</tt>, it can be used to enumerate the container's elements,
     * but in reverse.
     *
     * The iterator does not allow elements to be modified.
     *
     * @return @c const_reverse_iterator to end of container.
     * @remark This method can be used to get a @c const_reverse_iterator for a
     *         container even if the container itself is not @c const.
     * @see lazy_sorted_container::crbegin
     */
    const_reverse_iterator crend() const {
        sort_if_needed();
        return elements_.crend();
    }

    /**
     * @brief Checks if container is empty.
     *
     * Checks if the container is empty.
     *
     * @return @c true if the container has no element.
     */
    bool empty() const {
        return elements_.empty();
    }

    /**
     * @brief Returns container's size.
     *
     * Returns the number of elements stored in the container.
     *
     * @return Number of elements in the container.
     */
    size_type size() const {
        // If we're not sorted and this container does not accept duplicates, we have no choice but to sort.
        sort_lazy_container_if_needed_and_not_multi<Multi>()(*this);
        return elements_.size();
    }

    /**
     * @brief Returns container's max size.
     *
     * Returns the maximum number of elements that can be stored by the
     * container. This might be limited due to internal implementation,
     * element size, etc.
     *
     * @return Maximum number of elements that can be stored in the container.
     */
    size_type max_size() const {
        return elements_.max_size();
    }

    /**
     * @brief Reserves capacity in the container.
     *
     * Reserves memory in the container for the given number of elements.
     *
     * When planning to add a large number of elements whose count is
     * known in advance, reserving the space can save on reallocation.
     *
     * @param new_cap New required capacity. The final capacity after
     *                the call is implementation-specific, but guaranteed
     *                to be at least equal to @c new_cap.
     * @remark This method is only available if the internal container implementation
     *         (e.g. <tt>lazy_sorted_container::container_impl</tt>) supports it.
     */
    template<class = std::enable_if_t<has_reserve_method<container_impl, size_type>::value, void>>
    void reserve(size_type new_cap) {
        elements_.reserve(new_cap);
    }

    /**
     * @brief Returns container's capacity.
     *
     * Returns the container's capacity, e.g. how many elements it can contain
     * before having to reallocate memory somehow. This value will be greater
     * than or equal to that returned by <tt>size()</tt>.
     *
     * @return Container's current capacity.
     * @remark This method is only available if the internal container implementation
     *         (e.g. <tt>lazy_sorted_container::container_impl</tt>) supports it.
     */
    template<class = std::enable_if_t<has_capacity_const_method<container_impl>::value, void>>
    auto capacity() const {
        return elements_.capacity();
    }

    /**
     * @brief Optimizes memory for size.
     *
     * Asks the container to shrink its internal memory to the minimum required
     * to store its current elements. Depending on the internal container's
     * implementation, this might or might not produce a result.
     *
     * @remark This method is only available if the internal container implementation
     *         (e.g. <tt>lazy_sorted_container::container_impl</tt>) supports it.
     */
    template<class = std::enable_if_t<has_shrink_to_fit_method<container_impl>::value, void>>
    void shrink_to_fit() {
        elements_.shrink_to_fit();
    }

    /**
     * @brief Insert an element in the container.
     *
     * Inserts a new element in the container.
     *
     * @param value Element to insert.
     * @remark In order to support lazy sorting, this method returns
     *         @c void instead of a <tt>pair<iterator, bool></tt>.
     */
    void insert(const value_type& value) {
        // What we do is push new value in the internal container, then check if container is still sorted.
        elements_.push_back(value);
        if (sorted_) {
            update_sorted_after_push_back();
        }
    }

    /**
     * @brief Moves an element in the container.
     *
     * Inserts a new element in the container by moving it.
     * If the element supports move semantics, no copy is performed.
     *
     * @param value Element to move in the container.
     * @remark In order to support lazy sorting, this method returns
     *         @c void instead of a <tt>pair<iterator, bool></tt>.
     */
    void insert(value_type&& value) {
        elements_.push_back(std::move(value));
        if (sorted_) {
            update_sorted_after_push_back();
        }
    }

    /**
     * @brief Inserts an element in the container with a hint iterator.
     *
     * Inserts a new element in the container.
     *
     * This method accepts a hint iterator, but discards it in order
     * to support lazy sorting.
     *
     * @param hint Hint iterator; unused.
     * @param value Element to insert.
     * @remark In order to support lazy sorting, this method returns
     *         @c void instead of an iterator.
     */
    void insert(const_iterator hint, const value_type& value) {
        // Ignore hint iterator in favor of lazy sorting
        insert(value);
    }

    /**
     * @brief Moves an element in the container with a hint iterator.
     *
     * Inserts a new element in the container by moving it.
     * If the element supports move semantics, no copy is performed.
     *
     * This method accepts a hint iterator, but discards it in order
     * to support lazy sorting.
     *
     * @param hint Hint iterator; unused.
     * @param value Element to move in the container.
     * @remark In order to support lazy sorting, this method returns
     *         @c void instead of an iterator.
     */
    void insert(const_iterator hint, value_type&& value) {
        insert(std::move(value));
    }

    /**
     * @brief Inserts a range of elements in the container.
     *
     * Inserts all elements in the range <tt>[first, last[</tt>
     * in the container.
     *
     * @param first Beginning of range of elements to insert.
     * @param last End of range of elements to insert.
     */
    template<class It> void insert(It first, It last) {
        // Checking if container is sorted would be onerous for batch inserts.
        elements_.insert(elements_.cend(), first, last);
        sorted_ = elements_.size() <= 1;
    }

    /**
     * @brief Inserts elements from an initializer list in the container.
     *
     * Inserts all elements in the given @c initializer_list in
     * the container.
     *
     * @param init @c initializer_list containing the elements to insert.
     */
    void insert(std::initializer_list<value_type> init) {
        elements_.insert(elements_.cend(), std::begin(init), std::end(init));
        sorted_ = elements_.size() <= 1;
    }

    /**
     * @brief Constructs a new element in the container.
     *
     * Constructs a new element in the container by forwarding the
     * given arguments to <tt>lazy_sorted_container::value_type</tt>'s
     * constructor.
     *
     * Note that for maps, @c value_type is a pair and thus must
     * be constructed using one of <tt>std::pair</tt>'s constructors.
     *
     * @param args Arguments that will be forwarded to the element's constructor.
     * @remark In order to support lazy sorting, this method returns
     *         @c void instead of a <tt>pair<iterator, bool></tt>.
     */
    template<class... Args> void emplace(Args&&... args) {
        elements_.emplace_back(std::forward<Args>(args)...);
        if (sorted_) {
            update_sorted_after_push_back();
        }
    }
    
    /**
     * @brief Constructs a new element in the container with a hint iterator.
     *
     * Constructs a new element in the container by forwarding the
     * given arguments to <tt>lazy_sorted_container::value_type</tt>'s
     * constructor.
     *
     * This method accepts a hint iterator, but discards it in order
     * to support lazy sorting.
     *
     * Note that for maps, @c value_type is a pair and thus must
     * be constructed using one of <tt>std::pair</tt>'s constructors.
     *
     * @param args Arguments that will be forwarded to the element's constructor.
     * @remark In order to support lazy sorting, this method returns
     *         @c void instead of an @c iterator.
     */
    template<class... Args> void emplace_hint(const_iterator hint, Args&&... args) {
        // Ignore hint iterator in favor of lazy sorting
        emplace(std::forward<Args>(args)...);
    }

    /**
     * @brief Removes an element from the container.
     *
     * Removes the element pointed to by the given iterator from the container.
     *
     * @param pos Iterator pointing at element to remove.
     * @return Iterator pointing at element following the removed element or,
     *         if the removed element was the last one, at the end of the container.
     */
    iterator erase(const_iterator pos) {
        // If user has a valid iterator, it's because container is sorted.
        return elements_.erase(pos);
    }

    /**
     * @brief Removes many elements from the container.
     *
     * Removes all elements pointed to by iterators in the range
     * <tt>[first, last[</tt> from the container.
     *
     * @param first Beginning of range of elements to remove.
     * @param last End of range of elements to remove.
     * @return Iterator pointing at element following the last element removed or,
     *         if that was the last element, at the end of the container.
     */
    iterator erase(const_iterator first, const_iterator last) {
        return elements_.erase(first, last);
    }
    
    /**
     * @brief Removes elements by key.
     *
     * Removes all elements associated with the given key from the container.
     *
     * @param Key of element(s) to remove.
     * @return Number of elements removed. For containers that do not accept
     *         duplicates, this can never be greater than 1.
     */
    size_type erase(const key_type& key) {
        auto range = equal_range(key);
        size_type dist = std::distance(range.first, range.second);
        erase(range.first, range.second);
        return dist;
    }

    /**
     * @brief Clears all elements.
     *
     * Removes all elements from the container.
     */
    void clear() {
        elements_.clear();
        sorted_ = true;
    }

    /**
     * @brief Swaps the contents of two containers.
     *
     * Swaps the content of this container with another. After this method
     * returns, this container will contain the elements formely in @c obj
     * and vice-versa.
     *
     * Depending on internal container implementation, this is usually done
     * without moving of copying elements; only pointers are swapped.
     *
     * @param obj Container to swap with.
     */
    void swap(lazy_sorted_container& obj) {
        using std::swap;
        swap(elements_, obj.elements_);
        swap(sorted_, obj.sorted_);
        swap(vtok_, obj.vtok_);
        swap(vcmp_, obj.vcmp_);
        swap(veq_, obj.veq_);
    }

    /**
     * @brief Swaps the contents of two containers (ADL version).
     *
     * Swaps the content of two containers. After this function returns,
     * @c obj1 will contain the elements formely in @c obj2 and vice-versa.
     *
     * Depending on internal container implementation, this is usually done
     * without moving of copying elements; only pointers are swapped.
     *
     * This function is similar to the @c swap method, but can be found
     * through ADL:
     *
     * @code
     *   swap(c1, c2); // No need for coveo::lazy::...
     * @endcode
     *
     * @param obj1 First container to swap.
     * @param obj2 Second container to swap.
     */
    friend void swap(lazy_sorted_container& obj1, lazy_sorted_container& obj2) {
        obj1.swap(obj2);
    }

    /**
     * @brief Inserts or assigns a value.
     *
     * If the container contains an element associated with @c key, assigns
     * @c val (by forwarding it) to its associated value. Otherwise, inserts
     * a new element constructed by copying @c key and forwarding @c val.
     *
     * @param key Key of element to insert or assign to.
     * @param val Value to insert or assign to @c key.
     * @return @c pair whose @c first element is an @c iterator pointing at
     *         the element in the container and whose @c second element will
     *         be @c true if element was inserted or @c false if it was
     *         assigned to.
     * @remark This method is only available for non-multi maps.
     */
    template<class OT,
             bool _Enabled = _IsNonMultiMap,
             class = std::enable_if_t<_Enabled, void>>
    std::pair<iterator, bool> insert_or_assign(const key_type& key, OT&& val) {
        return insert_or_assign_impl(key, std::forward<OT>(val));
    }

    /**
     * @brief Inserts or assigns a value (move key version).
     *
     * If the container contains an element associated with @c key, assigns
     * @c val (by forwarding it) to its associated value. Otherwise, inserts
     * a new element constructed by moving @c key and forwarding @c val.
     *
     * @param key Key of element to move-insert or assign to.
     * @param val Value to insert or assign to @c key.
     * @return @c pair whose @c first element is an @c iterator pointing at
     *         the element in the container and whose @c second element will
     *         be @c true if element was inserted or @c false if it was
     *         assigned to.
     * @remark This method is only available for non-multi maps.
     */
    template<class OT,
             bool _Enabled = _IsNonMultiMap,
             class = std::enable_if_t<_Enabled, void>>
    std::pair<iterator, bool> insert_or_assign(key_type&& key, OT&& val) {
        return insert_or_assign_impl(std::move(key), std::forward<OT>(val));
    }

    /**
     * @brief Tries to construct an element in the container.
     *
     * If the container already contains an element associated with @c key,
     * does nothing; otherwise, constructs a new element in the container
     * by copying @c key and forwarding @c args to the value's constructor,
     * as if calling
     *
     * @code
     *   value_type(std::piecewise_construct,
     *              std::forward_as_tuple(key),
     *              std::forward_as_tuple(std::forward<Args>(args)...))
     * @endcode
     *
     * @param key Key of element to try adding.
     * @param args Arguments to forward to the value's constructor.
     * @return @c pair whose @c first element is an @c iterator pointing at
     *         the element in the container and whose @c second element will
     *         be @c true if element was added or @c false if it already existed.
     * @remark This method is only available for non-multi maps.
     */
    template<class... Args,
             bool _Enabled = _IsNonMultiMap,
             class = std::enable_if_t<_Enabled, void>>
    std::pair<iterator, bool> try_emplace(const key_type& key, Args&&... args) {
        return try_emplace_impl(key, std::forward<Args>(args)...);
    }

    /**
     * @brief Tries to construct an element in the container (move key version).
     *
     * If the container already contains an element associated with @c key,
     * does nothing; otherwise, constructs a new element in the container
     * by moving @c key and forwarding @c args to the value's constructor,
     * as if calling
     *
     * @code
     *   value_type(std::piecewise_construct,
     *              std::forward_as_tuple(std::move(key)),
     *              std::forward_as_tuple(std::forward<Args>(args)...))
     * @endcode
     *
     * @param key Key of element to try adding (by moving it).
     * @param args Arguments to forward to the value's constructor.
     * @return @c pair whose @c first element is an @c iterator pointing at
     *         the element in the container and whose @c second element will
     *         be @c true if element was added or @c false if it already existed.
     * @remark This method is only available for non-multi maps.
     */
    template<class... Args,
             bool _Enabled = _IsNonMultiMap,
             class = std::enable_if_t<_Enabled, void>>
    std::pair<iterator, bool> try_emplace(key_type&& key, Args&&... args) {
        return try_emplace_impl(std::move(key), std::forward<Args>(args)...);
    }

    /**
     * @brief Returns number of elements associated to a key.
     *
     * Returns the number of elements in the container that are associated
     * with @c key. For containers that do not accept duplicates, this
     * will be at most 1.
     *
     * @param key Key of element(s) to count.
     * @return Number of elements associated with @c key.
     */
    size_type count(const key_type& key) const {
        auto range = equal_range(key);
        return std::distance(range.first, range.second);
    }

    /**
     * @brief Looks for an element in the container.
     *
     * Looks for an element in the container associated with @c key.
     * If at least one is found, returns an <tt>lazy_sorted_container::iterator</tt>
     * pointing at the first such element, otherwise pointing at <tt>end()</tt>.
     *
     * @param key Key of element to look for.
     * @return @c iterator pointing at element if found, otherwise
     *         pointing at the @c end of the container.
     */
    iterator find(const key_type& key) {
        sort_if_needed();
        auto elem_end = elements_.end();
        auto it = std::lower_bound(elements_.begin(), elem_end, key, vcmp_);
        return (it != elem_end && !vcmp_(key, *it)) ? it : elem_end;
    }

    /**
     * @brief Looks for an element in the container (const version).
     *
     * Looks for an element in the container associated with @c key.
     * If at least one is found, returns a <tt>lazy_sorted_container::const_iterator</tt>
     * pointing at the first such element, otherwise pointing at <tt>cend()</tt>.
     *
     * @param key Key of element to look for.
     * @return @c const_iterator pointing at element if found, otherwise
     *         pointing at the @c end of the container.
     */
    const_iterator find(const key_type& key) const {
        sort_if_needed();
        auto elem_cend = elements_.cend();
        auto cit = std::lower_bound(elements_.cbegin(), elem_cend, key, vcmp_);
        return (cit != elem_cend && !vcmp_(key, *cit)) ? cit : elem_cend;
    }

    /**
     * @brief Looks for key's lower bound.
     *
     * Looks for the first element in the container whose key is greater than
     * or equal to @c key. This corresponds to the key's "lower bound".
     *
     * @param key Key to look for.
     * @return @c iterator pointing at first element whose key is greater than
     *         or equal to @c key. If no element is associated with @c key in
     *         the container, returns the first element associated with the
     *         next key or possibly <tt>end()</tt>.
     * @see lazy_sorted_container::upper_bound
     * @see lazy_sorted_container::equal_range
     */
    iterator lower_bound(const key_type& key) {
        sort_if_needed();
        return std::lower_bound(elements_.begin(), elements_.end(), key, vcmp_);
    }

    /**
     * @brief Looks for key's lower bound (const version).
     *
     * Looks for the first element in the container whose key is greater than
     * or equal to @c key. This corresponds to the key's "lower bound".
     * (@c const version)
     *
     * @param key Key to look for.
     * @return @c const_iterator pointing at first element whose key is greater than
     *         or equal to @c key. If no element is associated with @c key in
     *         the container, returns the first element associated with the
     *         next key or possibly <tt>cend()</tt>.
     * @see lazy_sorted_container::upper_bound
     * @see lazy_sorted_container::equal_range
     */
    const_iterator lower_bound(const key_type& key) const {
        sort_if_needed();
        return std::lower_bound(elements_.cbegin(), elements_.cend(), key, vcmp_);
    }

    /**
     * @brief Looks for key's upper bound.
     *
     * Looks for the first element in the container whose key is greater than @c key.
     * This corresponds to the key's "upper bound".
     *
     * @param key Key to look for.
     * @return @c iterator pointing at first element whose key is greater than @c key,
     *         possibly <tt>end()</tt>.
     * @see lazy_sorted_container::lower_bound
     * @see lazy_sorted_container::equal_range
     */
    iterator upper_bound(const key_type& key) {
        sort_if_needed();
        return std::upper_bound(elements_.begin(), elements_.end(), key, vcmp_);
    }

    /**
     * @brief Looks for key's upper bound (const version).
     *
     * Looks for the first element in the container whose key is greater than @c key.
     * This corresponds to the key's "upper bound". (@c const version)
     *
     * @param key Key to look for.
     * @return @c const_iterator pointing at first element whose key is greater than @c key,
     *         possibly <tt>cend()</tt>.
     * @see lazy_sorted_container::lower_bound
     * @see lazy_sorted_container::equal_range
     */
    const_iterator upper_bound(const key_type& key) const {
        sort_if_needed();
        return std::upper_bound(elements_.cbegin(), elements_.cend(), key, vcmp_);
    }

    /**
     * @brief Looks for key's lower and upper bounds.
     *
     * Looks for a key's lower and upper bounds. This is similar to
     * calling <tt>lower_bound()</tt> and <tt>upper_bound()</tt>.
     *
     * @param key Key to look for.
     * @return @c pair of <tt>iterator</tt>s, whose @c first points to the first
     *         element in the container whose key is greater than or equal
     *         to @c key and whose @c second points to the first element
     *         whose key is greater than @c key.
     * @see lazy_sorted_container::lower_bound
     * @see lazy_sorted_container::upper_bound
     */
    std::pair<iterator, iterator> equal_range(const key_type& key) {
        sort_if_needed();
        return std::equal_range(elements_.begin(), elements_.end(), key, vcmp_);
    }

    /**
     * @brief Looks for key's lower and upper bounds (const version).
     *
     * Looks for a key's lower and upper bounds. This is similar to
     * calling <tt>lower_bound()</tt> and <tt>upper_bound()</tt>.
     * (@c const version)
     *
     * @param key Key to look for.
     * @return @c pair of <tt>const_iterator</tt>s, whose @c first points to the
     *         first element in the container whose key is greater than or equal
     *         to @c key and whose @c second points to the first element
     *         whose key is greater than @c key.
     * @see lazy_sorted_container::lower_bound
     * @see lazy_sorted_container::upper_bound
     */
    std::pair<const_iterator, const_iterator> equal_range(const key_type& key) const {
        sort_if_needed();
        return std::equal_range(elements_.cbegin(), elements_.cend(), key, vcmp_);
    }

    /**
     * @brief Returns number of elements associated to a key using any type of key.
     *
     * Returns the number of elements in the container that are associated
     * with @c key, which can be any type accepted by
     * <tt>lazy_sorted_container::key_compare</tt>. For containers that
     * do not accept duplicates, this will be at most 1.
     *
     * @param key Key of element(s) to count.
     * @return Number of elements associated with @c key.
     * @remark This method is only available if <tt>lazy_sorted_container::key_compare</tt>
     *         is a transparent function (e.g. defines @c is_transparent), like
     *         <tt>std::less<></tt>. For more info, see
     *         http://en.cppreference.com/w/cpp/utility/functional/less_void
     */
    template<class OK,
             class _OKCmp = key_compare,
             class = typename _OKCmp::is_transparent>
    size_type count(const OK& key) const {
        auto range = equal_range(key);
        return std::distance(range.first, range.second);
    }

    /**
     * @brief Looks for an element in the container using any type of key.
     *
     * Looks for an element in the container associated with @c key, which
     * can be any type accepted by <tt>lazy_sorted_container::key_compare</tt>.
     * If at least one is found, returns an <tt>lazy_sorted_container::iterator</tt>
     * pointing at the first such element, otherwise pointing at <tt>end()</tt>.
     *
     * @param key Key of element to look for.
     * @return @c iterator pointing at element if found, otherwise
     *         pointing at the @c end of the container.
     * @remark This method is only available if <tt>lazy_sorted_container::key_compare</tt>
     *         is a transparent function (e.g. defines @c is_transparent), like
     *         <tt>std::less<></tt>. For more info, see
     *         http://en.cppreference.com/w/cpp/utility/functional/less_void
     */
    template<class OK,
             class _OKCmp = key_compare,
             class = typename _OKCmp::is_transparent>
    iterator find(const OK& key) {
        sort_if_needed();
        auto elem_end = elements_.end();
        auto it = std::lower_bound(elements_.begin(), elem_end, key, vcmp_);
        return (it != elem_end && !vcmp_(key, *it)) ? it : elem_end;
    }

    /**
     * @brief Looks for an element in the container using any type of key (const version).
     *
     * Looks for an element in the container associated with @c key, which
     * can be any type accepted by <tt>lazy_sorted_container::key_compare</tt>.
     * If at least one is found, returns a <tt>lazy_sorted_container::const_iterator</tt>
     * pointing at the first such element, otherwise pointing at <tt>cend()</tt>.
     *
     * @param key Key of element to look for.
     * @return @c const_iterator pointing at element if found, otherwise
     *         pointing at the @c end of the container.
     * @remark This method is only available if <tt>lazy_sorted_container::key_compare</tt>
     *         is a transparent function (e.g. defines @c is_transparent), like
     *         <tt>std::less<></tt>. For more info, see
     *         http://en.cppreference.com/w/cpp/utility/functional/less_void
     */
    template<class OK,
             class _OKCmp = key_compare,
             class = typename _OKCmp::is_transparent>
    const_iterator find(const OK& key) const {
        sort_if_needed();
        auto elem_cend = elements_.cend();
        auto cit = std::lower_bound(elements_.cbegin(), elem_cend, key, vcmp_);
        return (cit != elem_cend && !vcmp_(key, *cit)) ? cit : elem_cend;
    }

    /**
     * @brief Looks for key's lower bound using any type of key.
     *
     * Looks for the first element in the container whose key is greater
     * than or equal to @c key, which can be any type accepted by
     * <tt>lazy_sorted_container::key_compare</tt>. This corresponds
     * to the key's "lower bound".
     *
     * @param key Key to look for.
     * @return @c iterator pointing at first element whose key is greater than
     *         or equal to @c key. If no element is associated with @c key in
     *         the container, returns the first element associated with the
     *         next key or possibly <tt>end()</tt>.
     * @remark This method is only available if <tt>lazy_sorted_container::key_compare</tt>
     *         is a transparent function (e.g. defines @c is_transparent), like
     *         <tt>std::less<></tt>. For more info, see
     *         http://en.cppreference.com/w/cpp/utility/functional/less_void
     * @see lazy_sorted_container::upper_bound(const OK&)
     * @see lazy_sorted_container::equal_range(const OK&)
     */
    template<class OK,
             class _OKCmp = key_compare,
             class = typename _OKCmp::is_transparent>
    iterator lower_bound(const OK& key) {
        sort_if_needed();
        return std::lower_bound(elements_.begin(), elements_.end(), key, vcmp_);
    }

    /**
     * @brief Looks for key's lower bound using any type of key (const version).
     *
     * Looks for the first element in the container whose key is greater
     * than or equal to @c key, which can be any type accepted by
     * <tt>lazy_sorted_container::key_compare</tt>. This corresponds
     * to the key's "lower bound". (@c const version)
     *
     * @param key Key to look for.
     * @return @c const_iterator pointing at first element whose key is greater than
     *         or equal to @c key. If no element is associated with @c key in
     *         the container, returns the first element associated with the
     *         next key or possibly <tt>cend()</tt>.
     * @remark This method is only available if <tt>lazy_sorted_container::key_compare</tt>
     *         is a transparent function (e.g. defines @c is_transparent), like
     *         <tt>std::less<></tt>. For more info, see
     *         http://en.cppreference.com/w/cpp/utility/functional/less_void
     * @see lazy_sorted_container::upper_bound(const OK&)
     * @see lazy_sorted_container::equal_range(const OK&)
     */
    template<class OK,
             class _OKCmp = key_compare,
             class = typename _OKCmp::is_transparent>
    const_iterator lower_bound(const OK& key) const {
        sort_if_needed();
        return std::lower_bound(elements_.cbegin(), elements_.cend(), key, vcmp_);
    }

    /**
     * @brief Looks for key's upper bound using any type of key.
     *
     * Looks for the first element in the container whose key is greater than @c key,
     * which can be any type accepted by <tt>lazy_sorted_container::key_compare</tt>.
     * This corresponds to the key's "upper bound".
     *
     * @param key Key to look for.
     * @return @c iterator pointing at first element whose key is greater than @c key,
     *         possibly <tt>end()</tt>.
     * @remark This method is only available if <tt>lazy_sorted_container::key_compare</tt>
     *         is a transparent function (e.g. defines @c is_transparent), like
     *         <tt>std::less<></tt>. For more info, see
     *         http://en.cppreference.com/w/cpp/utility/functional/less_void
     * @see lazy_sorted_container::lower_bound(const OK&)
     * @see lazy_sorted_container::equal_range(const OK&)
     */
    template<class OK,
             class _OKCmp = key_compare,
             class = typename _OKCmp::is_transparent>
    iterator upper_bound(const OK& key) {
        sort_if_needed();
        return std::upper_bound(elements_.begin(), elements_.end(), key, vcmp_);
    }

    /**
     * @brief Looks for key's upper bound using any type of key (const version).
     *
     * Looks for the first element in the container whose key is greater than @c key,
     * which can be any type accepted by <tt>lazy_sorted_container::key_compare</tt>.
     * This corresponds to the key's "upper bound". (@c const version)
     *
     * @param key Key to look for.
     * @return @c const_iterator pointing at first element whose key is greater than @c key,
     *         possibly <tt>cend()</tt>.
     * @remark This method is only available if <tt>lazy_sorted_container::key_compare</tt>
     *         is a transparent function (e.g. defines @c is_transparent), like
     *         <tt>std::less<></tt>. For more info, see
     *         http://en.cppreference.com/w/cpp/utility/functional/less_void
     * @see lazy_sorted_container::lower_bound(const OK&)
     * @see lazy_sorted_container::equal_range(const OK&)
     */
    template<class OK,
             class _OKCmp = key_compare,
             class = typename _OKCmp::is_transparent>
    const_iterator upper_bound(const OK& key) const {
        sort_if_needed();
        return std::upper_bound(elements_.cbegin(), elements_.cend(), key, vcmp_);
    }

    /**
     * @brief Looks for key's lower and upper bounds using any type of key.
     *
     * Looks for a key's lower and upper bounds using any type accepted
     * by <tt>lazy_sorted_container::key_compare</tt>. This is similar to
     * calling <tt>lower_bound()</tt> and <tt>upper_bound()</tt>.
     *
     * @param key Key to look for.
     * @return @c pair of <tt>iterator</tt>s, whose @c first points to the first
     *         element in the container whose key is greater than or equal
     *         to @c key and whose @c second points to the first element
     *         whose key is greater than @c key.
     * @remark This method is only available if <tt>lazy_sorted_container::key_compare</tt>
     *         is a transparent function (e.g. defines @c is_transparent), like
     *         <tt>std::less<></tt>. For more info, see
     *         http://en.cppreference.com/w/cpp/utility/functional/less_void
     * @see lazy_sorted_container::lower_bound(const OK&)
     * @see lazy_sorted_container::upper_bound(const OK&)
     */
    template<class OK,
             class _OKCmp = key_compare,
             class = typename _OKCmp::is_transparent>
    std::pair<iterator, iterator> equal_range(const OK& key) {
        sort_if_needed();
        return std::equal_range(elements_.begin(), elements_.end(), key, vcmp_);
    }

    /**
     * @brief Looks for key's lower and upper bounds using any type of key (const version).
     *
     * Looks for a key's lower and upper bounds using any type accepted
     * by <tt>lazy_sorted_container::key_compare</tt>. This is similar to
     * calling <tt>lower_bound()</tt> and <tt>upper_bound()</tt>.
     *
     * @param key Key to look for.
     * @return @c pair of <tt>const_iterator</tt>s, whose @c first points to the first
     *         element in the container whose key is greater than or equal
     *         to @c key and whose @c second points to the first element
     *         whose key is greater than @c key.
     * @remark This method is only available if <tt>lazy_sorted_container::key_compare</tt>
     *         is a transparent function (e.g. defines @c is_transparent), like
     *         <tt>std::less<></tt>. For more info, see
     *         http://en.cppreference.com/w/cpp/utility/functional/less_void
     * @see lazy_sorted_container::lower_bound(const OK&)
     * @see lazy_sorted_container::upper_bound(const OK&)
     */
    template<class OK,
             class _OKCmp = key_compare,
             class = typename _OKCmp::is_transparent>
    std::pair<const_iterator, const_iterator> equal_range(const OK& key) const {
        sort_if_needed();
        return std::equal_range(elements_.cbegin(), elements_.cend(), key, vcmp_);
    }

    /**
     * @brief Returns key comparator.
     *
     * Returns the instance of <tt>lazy_sorted_container::key_compare</tt>
     * used by the container to compare keys.
     *
     * @return Key comparator instance.
     */
    key_compare key_comp() const {
        return vcmp_.key_predicate();
    }

    /**
     * @brief Returns value comparator.
     *
     * Returns the instance of <tt>lazy_sorted_container::value_compare</tt>
     * used by the container to compare elements, or "values".
     *
     * @return Value comparator instance.
     */
    value_compare value_comp() const {
        return vcmp_;
    }

    /**
     * @brief Returns allocator.
     *
     * Returns the instance of <tt>lazy_sorted_container::allocator_type</tt>
     * used by the container to allocate memory.
     *
     * @return Allocator instance.
     */
    allocator_type get_allocator() const {
        return elements_.get_allocator();
    }

    /**
     * @brief Returns key equality predicate.
     *
     * Returns the instance of <tt>lazy_sorted_container::key_equal_to</tt>
     * used by the container to determine if two keys are equal.
     *
     * @return Key equality predicate instance.
     */
    key_equal_to key_eq() const {
        return veq_.key_predicate();
    }

    /**
     * @brief Returns value equality predicate.
     *
     * Returns the instance of <tt>lazy_sorted_container::value_equal_to</tt>
     * used by the container to determine if two elements, or "values",
     * are equal.
     *
     * @return Value equality predicate instance.
     */
    value_equal_to value_eq() const {
        return veq_;
    }

    /**
     * @brief Checks if container is sorted.
     *
     * Determines if the elements in the container are currently sorted.
     * Elements could be unsorted following insertions, if no query
     * was performed.
     *
     * It's possible to trigger sorting by calling <tt>sort()</tt>.
     *
     * @return @c true if elements are currently sorted.
     * @see lazy_sorted_container::sort
     */
    bool sorted() const {
        return sorted_;
    }

    /**
     * @brief Forces container to sort elements.
     *
     * Forces container to sort its elements immediately. Has no effect
     * if elements are already sorted.
     *
     * @see lazy_sorted_container::sorted
     */
    void sort() {
        sort_if_needed();
    }
    
private:
    // Internal sorting
    void sort_if_needed() const {
        // This method is const because we need to be able to perform sorting even in const
        // lookup/iteration methods. elements_ and sorted_ are mutable to make this possible.
        if (!sorted_) {
            // Wrap actual sorting in another method to try and improve inlining.
            internal_sort();
        }
    }
    void internal_sort() const {
        // Sort, then remove duplicates if container does not accept them.
        sort_lazy_container_elements<Multi>()(*this);
        sorted_ = true;
    }

    // Internal method to keep sorted if possible
    void update_sorted_after_push_back() {
        if (elements_.size() > 1) {
            // Keep sorted if new element was inserted in the proper place.
            updated_lazy_container_sorted_flag_after_insert<Multi>()(*this);
        }
    }

    // Internal implementation of operator[]. Works with both lvalue and rvalue references.
    template<class OK,
             class _T = T,
             class _TRef = std::enable_if_t<_IsNonMultiMap && std::is_same<std::decay_t<OK>, key_type>::value, _T&>>
    _TRef operator_brackets_impl(OK&& key) {
        sort_if_needed();
        auto elem_end = elements_.end();
        auto it = std::lower_bound(elements_.begin(), elem_end, key, vcmp_);
        if (it == elem_end || vcmp_(key, *it)) {
            it = elements_.emplace(it, std::piecewise_construct,
                                       std::forward_as_tuple(std::forward<OK>(key)),
                                       std::make_tuple());
        }
        return it->second;
    }

    // Internal implementation of insert_or_assign(). Works with both lvalue and rvalue key references.
    template<class OK,
             class OT,
             class = std::enable_if_t<_IsNonMultiMap && std::is_same<std::decay_t<OK>, key_type>::value, void>>
    std::pair<iterator, bool> insert_or_assign_impl(OK&& key, OT&& val) {
        sort_if_needed();
        auto elem_end = elements_.end();
        auto it = std::lower_bound(elements_.begin(), elem_end, key, vcmp_);
        bool inserted = (it == elem_end || vcmp_(key, *it));
        if (inserted) {
            // Element does not exist, we must insert.
            it = elements_.emplace(it, std::forward<OK>(key), std::forward<OT>(val));
        } else {
            // Element already exists, assign to existing mapped value.
            it->second = std::forward<OT>(val);
        }
        return std::pair<iterator, bool>(std::move(it), inserted);
    }

    // Internal implementation of try_emplace(). Works with both lvalue and rvalue key references.
    template<class OK,
             class... Args,
             class = std::enable_if_t<_IsNonMultiMap && std::is_same<std::decay_t<OK>, key_type>::value, void>>
    std::pair<iterator, bool> try_emplace_impl(OK&& key, Args&&... args) {
        sort_if_needed();
        auto elem_end = elements_.end();
        auto it = std::lower_bound(elements_.begin(), elem_end, key, vcmp_);
        bool emplaced = (it == elem_end || vcmp_(key, *it));
        if (emplaced) {
            // Element doesn't exist, we can emplace.
            it = elements_.emplace(it, std::piecewise_construct,
                                       std::forward_as_tuple(std::forward<OK>(key)),
                                       std::forward_as_tuple(std::forward<Args>(args)...));
        }
        return std::pair<iterator, bool>(std::move(it), emplaced);
    }

    // Helpers to throw consistent exceptions
    [[noreturn]] void throw_out_of_range() const {
        throw coveo::lazy::out_of_range("out_of_range");
    }
};
/**
 * @internal
 * @brief Predicate that implements "equal to" using a comparator like <tt>std::less</tt>.
 * @headerfile lazy_sorted_container.h <coveo/lazy/detail/lazy_sorted_container.h>
 *
 * @tparam T Type of elements to compare.
 * @tparam _Cmp Comparator to use to implement equality. Defaults to <tt>std::less<T></tt>.
 */
template<class T,
         class _Cmp = std::less<T>>
class equal_to_using_less
{
    _Cmp cmp_;
public:
    equal_to_using_less() = default;
    explicit equal_to_using_less(const _Cmp& cmp) : cmp_(cmp) { }
    explicit equal_to_using_less(_Cmp&& cmp) : cmp_(std::move(cmp)) { }

    bool operator()(const T& left, const T& right) const {
        return !cmp_(left, right) && !cmp_(right, left);
    }
};

/**
 * @internal
 * @brief Internal proxy for <tt>std::equal_to</tt>.
 * @headerfile lazy_sorted_container.h <coveo/lazy/detail/lazy_sorted_container.h>
 *
 * Proxy for <tt>std::equal_to</tt> that has a constructor that accepts an arbitrary
 * unused predicate. Used to bridge with @c equal_to_using_less.
 *
 * @tparam T Type of elements to compare.
 * @tparam _UnusedCmp Comparator to use to implement equality; unused.
 */
template<class T,
         class _UnusedCmp = std::less<T>>
struct equal_to_proxy : std::equal_to<T> {
    equal_to_proxy() = default;
    explicit equal_to_proxy(const _UnusedCmp&) : equal_to_proxy() { }
};

/// @cond NEVERSHOWN

/**
 * @internal
 * @brief "Equal to" implementation that tries to be efficient.
 * @headerfile lazy_sorted_container.h <coveo/lazy/detail/lazy_sorted_container.h>
 *
 * "Equal to" predicate that uses <tt>operator==()</tt> if it's available for the type,
 * otherwise implements it using @c equal_to_using_less.
 *
 * @tparam T Type of elements to compare.
 * @tparam _Cmp Comparator to use to implement equality if <tt>operator==()</tt>
 *              is not available. Defaults to <tt>std::less<T></tt>.
 */
template<class T,
         class _Cmp = std::less<T>>
using equal_to_using_less_if_needed = std::conditional_t<has_operator_equal<T>::value,
                                                         equal_to_proxy<T, _Cmp>,
                                                         equal_to_using_less<T, _Cmp>>;

/// @endcond

/**
 * @internal
 * @brief Predicate that projects identity.
 * @headerfile lazy_sorted_container.h <coveo/lazy/detail/lazy_sorted_container.h>
 *
 * Unary predicate that projects a value reference unmodified.
 * Used as "value "to key" for set-like containers.
 *
 * @tparam T Type of value to project.
 */
template<class T>
struct identity {
    const T& operator()(const T& obj) const {
        return obj;
    }
};

/**
 * @internal
 * @brief Predicate that projects a pair's first element.
 * @headerfile lazy_sorted_container.h <coveo/lazy/detail/lazy_sorted_container.h>
 *
 * Unary predicate that projects the reference to a <tt>pair</tt>'s @c first element.
 * Used as "value to key" for map-like containers.
 *
 * @tparam P Type of pair to project the first element of.
 */
template<class P>
struct pair_first {
    decltype(auto) operator()(const P& obj) const {
        return std::get<0>(obj);
    }
};

/**
 * @internal
 * @brief Specialization of <tt>std::pair</tt> for map-like lazy-sorted containers.
 * @headerfile lazy_sorted_container.h <coveo/lazy/detail/lazy_sorted_container.h>
 *
 * Specialization of <tt>std::pair</tt> that supports assigning to the first element
 * even if it is const. This will be used to build lazy <tt>std::map</tt>-like
 * containers that can copy/move their elements in their internal container.
 *
 * The pair's first element is implicitely @c const. Use like this:
 *
 * @code
 *    detail::map_pair<int, float>  // inherits from std::pair<const int, float>
 * @endcode
 *
 * @tparam RT1 Type for the pair's first element. Implicitely @c const (see above).
 * @tparam T2 Type for the pair's second selement.
 */
template<class RT1,
         class T2,
         class _BaseStdPair = std::pair<const RT1, T2>>
class map_pair : public _BaseStdPair
{
public:
    // Typedef that can be used to refer to our base std::pair class.
    using base_std_pair = _BaseStdPair;

private:
    // Helper constructor called by the piecewise constructor.
    // This trick is used to be able to get an integer_sequence
    // representing the indexes of arguments in the parameter pack.
    template<class Args1, class Args2, size_t... Indexes1, size_t... Indexes2>
    map_pair(Args1& fir_args, Args2& sec_args,
             std::index_sequence<Indexes1...>,
             std::index_sequence<Indexes2...>)
        : base_std_pair(std::piecewise_construct,
                        std::forward_as_tuple(std::get<Indexes1>(std::move(fir_args))...),
                        std::forward_as_tuple(std::get<Indexes2>(std::move(sec_args))...)) {
#ifdef _MSC_VER
        // Visual Studio outputs warnings if either tuple has no element because
        // it ends up not being expanded at all in that case.
        fir_args;
        sec_args;
#endif // _MSC_VER
    }

public:
    // Default constructor
    map_pair() = default;

    // Constructors with parameters for first and second.
    map_pair(const RT1& fir, const T2& sec) : base_std_pair(fir, sec) { }
    template<class U1, class U2>
    map_pair(U1&& fir, U2&& sec)
        : base_std_pair(std::forward<U1>(fir), std::forward<U2>(sec)) { }

    // Copy and move constructors.
    map_pair(const map_pair&) = default;
    map_pair(map_pair&& obj)
        : base_std_pair(std::move(const_cast<RT1&>(obj.first)), std::move(obj.second)) { }

    // Constructors from compatible map_pair's.
    template<class RU1, class U2>
    map_pair(const map_pair<RU1, U2>& obj) : base_std_pair(obj) { }
    template<class RU1, class U2>
    map_pair(map_pair<RU1, U2>&& obj)
        : base_std_pair(std::move(const_cast<RU1&>(obj.first)), std::move(obj.second)) { }

    // Constructors from std::pair.
    template<class U1, class U2>
    map_pair(const std::pair<U1, U2>& obj) : base_std_pair(obj) { }
    template<class U1, class U2>
    map_pair(std::pair<U1, U2>&& obj)
        : base_std_pair(std::move(const_cast<std::remove_reference_t<U1>&>(obj.first)), std::move(obj.second)) { }

    // Piecewise constructor. Calls the private helper constructor (see above)
    // to avoid copying/moving objects in the tuple one too many time.
    template<class... Args1, class... Args2>
    map_pair(std::piecewise_construct_t,
             std::tuple<Args1...> fir_args,
             std::tuple<Args2...> sec_args)
        : map_pair(fir_args, sec_args, std::index_sequence_for<Args1...>(), std::index_sequence_for<Args2...>()) { }

    // Assignment operators.
    map_pair& operator=(const map_pair& obj) {
        if (this != &obj) {
            const_cast<RT1&>(this->first) = obj.first;
            this->second = obj.second;
        }
        return *this;
    }
    template<class RU1, class U2>
    map_pair& operator=(const map_pair<RU1, U2>& obj) {
        const_cast<RT1&>(this->first) = obj.first;
        this->second = obj.second;
        return *this;
    }

    // Move assignment operators.
    map_pair& operator=(map_pair&& obj) {
        const_cast<RT1&>(this->first) = std::move(const_cast<RT1&>(obj.first));
        this->second = std::move(obj.second);
        return *this;
    }
    template<class RU1, class U2>
    map_pair& operator=(map_pair<RU1, U2>&& obj) {
        const_cast<RT1&>(this->first) = std::move(const_cast<RU1&>(obj.first));
        this->second = std::move(obj.second);
        return *this;
    }

    // Assignment operator from std::pair.
    template<class U1, class U2>
    map_pair& operator=(const std::pair<U1, U2>& obj) {
        const_cast<RT1&>(this->first) = obj.first;
        this->second = obj.second;
        return *this;
    }

    // Move assignment operator from std::pair.
    template<class U1, class U2>
    map_pair& operator=(std::pair<U1, U2>&& obj) {
        const_cast<RT1&>(this->first) = std::move(const_cast<std::remove_reference_t<U1>&>(obj.first));
        this->second = std::move(obj.second);
        return *this;
    }

    // Swap
    void swap(map_pair& right) {
        using std::swap;
        swap(const_cast<RT1&>(this->first), const_cast<RT1&>(right.first));
        swap(this->second, right.second);
    }
    // Non-member swap that can be found via ADL
    friend void swap(map_pair& left, map_pair& right) {
        left.swap(right);
    }
};

/// @cond NEVERSHOWN

/**
 * @internal
 * @brief Default allocator for lazy-sorted map containers.
 * @headerfile lazy_sorted_container.h <coveo/lazy/detail/lazy_sorted_container.h>
 *
 * Default allocator type for lazy maps that use @c map_pair. Will be brought
 * into the <tt>coveo::lazy</tt> namespace in the <tt>map.h</tt> header. Can be used
 * to declare lazy map classes with custom allocators without having to reference
 * the internal @c map_pair class:
 *
 * @code
 *   // my_custom_allocator must be a template accepting a single type argument
 *   coveo::lazy::map_allocator<key_type, mapped_type, my_custom_allocator>
 * @endcode
 *
 * @tparam K Type for the map pair's first element.
 * @tparam T Type for the map pair's second element.
 * @tparam _Alloc Type of allocator to use. Defaults to <tt>std::allocator</tt>.
 */
template<class K,
         class T,
         template<class _AllocT> class _Alloc = std::allocator>
using map_allocator = _Alloc<map_pair<K, T>>;

/// @endcond

} // detail
} // lazy
} // coveo

#endif // COVEO_LAZY_DETAIL_LAZY_SORTED_CONTAINER_H

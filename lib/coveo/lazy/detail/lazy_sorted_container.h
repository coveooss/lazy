// Copyright (c) 2015-2016, Coveo Solutions Inc.
// Distributed under the Apache License, Version 2.0 (see LICENSE).

// This file contains the internal implementation common to all lazy-sorted
// containers as well as other internal types/functions as needed.

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

// Incomplete type to make some template usage fail.
struct incomplete_type;

// Type trait that can be used to know if there is a usable operator==()
// for a given type. Detects both member and non-member functions.
// Inspired by http://stackoverflow.com/a/257382 and others.
template<typename T>
class has_operator_equal
{
    static_assert(sizeof(std::int_least8_t) != sizeof(std::int_least32_t),
                  "has_operator_equal only works if int_least8_t has a different size than int_least32_t");

    template<typename C> static std::int_least8_t  test(decltype(std::declval<C>() == std::declval<C>())*); // Will be selected if operator== exists for C
    template<typename C> static std::int_least32_t test(...);                                               // Will be selected otherwise
public:
    static const bool value = sizeof(test<T>(nullptr)) == sizeof(std::int_least8_t);
};

// Type trait that can be used to know if a type has a reserve() method
// that is callable with a single argument of the given type.
template<typename T, typename SizeT>
class has_reserve_method
{
    static_assert(sizeof(std::int_least8_t) != sizeof(std::int_least32_t),
                  "has_reserve_method only works if int_least8_t has a different size than int_least32_t");

    template<typename C> static std::int_least8_t  test(decltype(std::declval<C>().reserve(std::declval<SizeT>()))*);   // Will be selected if C has reserve() accepting SizeT
    template<typename C> static std::int_least32_t test(...);                                                           // Will be selected otherwise
public:
    static const bool value = sizeof(test<T>(nullptr)) == sizeof(std::int_least8_t);
};

// Type trait that can be used to know if a type has a capacity() const method
// that returns a non-void result.
template<typename T>
class has_capacity_const_method
{
    static_assert(sizeof(std::int_least8_t) != sizeof(std::int_least32_t),
                  "has_capacity_method only works if int_least8_t has a different size than int_least32_t");

    template<typename C> static std::int_least8_t  test(std::enable_if_t<!std::is_void<decltype(std::declval<const C>().capacity())>::value, void*>);   // Will be selected if C has capacity() that does not return void
    template<typename C> static std::int_least32_t test(...);                                                                                           // Will be selected otherwise
public:
    static const bool value = sizeof(test<T>(nullptr)) == sizeof(std::int_least8_t);
};

// Type trait that can be used to know if a type has a shrink_to_fit() method.
template<typename T>
class has_shrink_to_fit_method
{
    static_assert(sizeof(std::int_least8_t) != sizeof(std::int_least32_t),
                  "has_shrink_to_fit_method only works if int_least8_t has a different size than int_least32_t");

    template<typename C> static std::int_least8_t  test(decltype(std::declval<C>().shrink_to_fit())*);  // Will be selected if C has shrink_to_fit()
    template<typename C> static std::int_least32_t test(...);                                           // Will be selected otherwise
public:
    static const bool value = sizeof(test<T>(nullptr)) == sizeof(std::int_least8_t);
};

// Proxy predicate that can act on values in a lazy sorted container by using
// a "value to key" function object to forward the values' keys to a key-based
// predicate. Can also act on any object that can be interpreted as a "key" by
// the key-based predicate.
//
// Template arguments:
// V     - Type of values passed to this predicate.
// VToK  - Predicate that can extract a key for a value.
// KPred - Key-based predicate to proxy.
template<typename V,
         typename VToK,
         typename KPred>
class lazy_value_pred_proxy
{
    VToK vtok_;
    KPred kpr_;
public:
    template<typename OVToK, typename OKPred>
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
    template<typename OK,
             typename = std::enable_if_t<!std::is_same<OK, V>::value && !std::is_base_of<V, OK>::value, void>>
    decltype(auto) operator()(const V& left, const OK& rightk) const {
        return kpr_(vtok_(left), rightk);
    }
    template<typename OK,
             typename = std::enable_if_t<!std::is_same<OK, V>::value && !std::is_base_of<V, OK>::value, void>>
    decltype(auto) operator()(const OK& leftk, const V& right) const {
        return kpr_(leftk, vtok_(right));
    }
};

// Forward iterator proxy that wraps another forward iterator but converts the
// reference returned by operator*() to another reference type.
//
// Template arguments:
// It     - Type of iterator to proxy.
// V      - Type of values returned by It.
// PubV   - Type of values to publicly return references of. Must be either
//          V or a base class of V.
// _ItCat - Iterator category for It. Uses std::iterator_traits by default.
// _Diff  - Type used for differences between iterators. Uses
//          std::iterator_traits by default.
template<typename It,
         typename V,
         typename PubV,
         typename _ItCat = typename std::iterator_traits<It>::iterator_category,
         typename _Diff = typename std::iterator_traits<It>::difference_type>
class forward_iterator_proxy : public std::iterator<_ItCat, PubV, _Diff, PubV*, PubV&>
{
    static_assert(std::is_base_of<std::forward_iterator_tag, _ItCat>::value,
                  "forward_iterator_proxy can only wrap forward iterators");
    static_assert(std::is_same<PubV, V>::value || std::is_base_of<PubV, V>::value,
                  "forward_iterator_proxy can only return references to base classes of their wrapped iterator's value_type");

    // Helper used to identify our type
    template<typename T> struct is_forward_iterator_proxy : std::false_type { };
    template<typename OIt, typename OV, typename OPubV, typename _OItCat, typename _ODiff>
    struct is_forward_iterator_proxy<forward_iterator_proxy<OIt, OV, OPubV, _OItCat, _ODiff>> : std::true_type { };
    
    // Friend compatible iterators to be able to access it_
    template<typename OIt, typename OV, typename OPubV, typename _OItCat, typename _ODiff>
    friend class forward_iterator_proxy;
protected:
    It it_; // Iterator being proxied

public:
    forward_iterator_proxy() = default;
    forward_iterator_proxy(const forward_iterator_proxy& other) = default;
    forward_iterator_proxy(forward_iterator_proxy&& other) = default;
    template<typename OIt, typename OV, typename OPubV, typename _OItCat, typename _ODiff>
    forward_iterator_proxy(const forward_iterator_proxy<OIt, OV, OPubV, _OItCat, _ODiff>& other)
        : it_(other.it_) { }
    template<typename OIt, typename OV, typename OPubV, typename _OItCat, typename _ODiff>
    forward_iterator_proxy(forward_iterator_proxy<OIt, OV, OPubV, _OItCat, _ODiff>&& other)
        : it_(std::move(other.it_)) { }
    template<typename T,
             typename = std::enable_if_t<!is_forward_iterator_proxy<std::decay_t<T>>::value, void>>
    forward_iterator_proxy(T&& obj) : it_(std::forward<T>(obj)) { }
    template<typename T1, typename T2, typename... Tx>
    forward_iterator_proxy(T1&& obj1, T2&& obj2, Tx&&... objx)
        : it_(std::forward<T1>(obj1), std::forward<T2>(obj2), std::forward<Tx>(objx)...) { }

    forward_iterator_proxy& operator=(const forward_iterator_proxy& other) = default;
    forward_iterator_proxy& operator=(forward_iterator_proxy&& other) = default;
    template<typename OIt, typename OV, typename OPubV, typename _OItCat, typename _ODiff>
    forward_iterator_proxy& operator=(const forward_iterator_proxy<OIt, OV, OPubV, _OItCat, _ODiff>& other) {
        it_ = other.it_;
        return *this;
    }
    template<typename OIt, typename OV, typename OPubV, typename _OItCat, typename _ODiff>
    forward_iterator_proxy& operator=(forward_iterator_proxy<OIt, OV, OPubV, _OItCat, _ODiff>&& other) {
        it_ = std::move(other.it_);
        return *this;
    }
    template<typename T,
             typename = std::enable_if_t<!is_forward_iterator_proxy<std::decay_t<T>>::value, void>>
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

// Bidirectional iterator proxy that wraps another bidirectional iterator but
// converts the reference returned by operator*() to another reference type.
//
// Template arguments:
// It     - Type of iterator to proxy.
// V      - Type of values returned by It.
// PubV   - Type of values to publicly return references of. Must be either
//          V or a base class of V.
// _ItCat - Iterator category for It. Uses std::iterator_traits by default.
// _Diff  - Type used for differences between iterators. Uses
//          std::iterator_traits by default.
template<typename It,
         typename V,
         typename PubV,
         typename _ItCat = typename std::iterator_traits<It>::iterator_category,
         typename _Diff = typename std::iterator_traits<It>::difference_type>
class bidirectional_iterator_proxy : public forward_iterator_proxy<It, V, PubV, _ItCat, _Diff>
{
    static_assert(std::is_base_of<std::bidirectional_iterator_tag, _ItCat>::value,
                  "bidirectional_iterator_proxy can only wrap bidirectional iterators");

    // Helper used to identify our type
    template<typename T> struct is_bidirectional_iterator_proxy : std::false_type { };
    template<typename OIt, typename OV, typename OPubV, typename _OItCat, typename _ODiff>
    struct is_bidirectional_iterator_proxy<bidirectional_iterator_proxy<OIt, OV, OPubV, _OItCat, _ODiff>> : std::true_type { };
public:
    bidirectional_iterator_proxy() = default;
    bidirectional_iterator_proxy(const bidirectional_iterator_proxy& other) = default;
    bidirectional_iterator_proxy(bidirectional_iterator_proxy&& other) = default;
    template<typename OIt, typename OV, typename OPubV, typename _OItCat, typename _ODiff>
    bidirectional_iterator_proxy(const bidirectional_iterator_proxy<OIt, OV, OPubV, _OItCat, _ODiff>& other)
        : forward_iterator_proxy<It, V, PubV, _ItCat, _Diff>(static_cast<const forward_iterator_proxy<OIt, OV, OPubV, _OItCat, _ODiff>&>(other)) { }
    template<typename OIt, typename OV, typename OPubV, typename _OItCat, typename _ODiff>
    bidirectional_iterator_proxy(bidirectional_iterator_proxy<OIt, OV, OPubV, _OItCat, _ODiff>&& other)
        : forward_iterator_proxy<It, V, PubV, _ItCat, _Diff>(static_cast<forward_iterator_proxy<OIt, OV, OPubV, _OItCat, _ODiff>&&>(other)) { }
    template<typename T,
             typename = std::enable_if_t<!is_bidirectional_iterator_proxy<std::decay_t<T>>::value, void>>
    bidirectional_iterator_proxy(T&& obj)
        : forward_iterator_proxy<It, V, PubV, _ItCat, _Diff>(std::forward<T>(obj)) { }
    template<typename T1, typename T2, typename... Tx>
    bidirectional_iterator_proxy(T1&& obj1, T2&& obj2, Tx&&... objx)
        : forward_iterator_proxy<It, V, PubV, _ItCat, _Diff>(std::forward<T1>(obj1), std::forward<T2>(obj2), std::forward<Tx>(objx)...) { }

    bidirectional_iterator_proxy& operator=(const bidirectional_iterator_proxy& other) = default;
    bidirectional_iterator_proxy& operator=(bidirectional_iterator_proxy&& other) = default;
    template<typename OIt, typename OV, typename OPubV, typename _OItCat, typename _ODiff>
    bidirectional_iterator_proxy& operator=(const bidirectional_iterator_proxy<OIt, OV, OPubV, _OItCat, _ODiff>& other) {
        forward_iterator_proxy<It, V, PubV, _ItCat, _Diff>::operator=(other);
        return *this;
    }
    template<typename OIt, typename OV, typename OPubV, typename _OItCat, typename _ODiff>
    bidirectional_iterator_proxy& operator=(bidirectional_iterator_proxy<OIt, OV, OPubV, _OItCat, _ODiff>&& other) {
        forward_iterator_proxy<It, V, PubV, _ItCat, _Diff>::operator=(std::move(other));
        return *this;
    }
    template<typename T,
             typename = std::enable_if_t<!is_bidirectional_iterator_proxy<std::decay_t<T>>::value, void>>
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

// Random-access iterator proxy that wraps another random-access iterator but
// converts the reference returned by operator*() to another reference type.
//
// Template arguments:
// It     - Type of iterator to proxy.
// V      - Type of values returned by It.
// PubV   - Type of values to publicly return references of. Must be either
//          V or a base class of V.
// _ItCat - Iterator category for It. Uses std::iterator_traits by default.
// _Diff  - Type used for differences between iterators. Uses
//          std::iterator_traits by default.
template<typename It,
         typename V,
         typename PubV,
         typename _ItCat = typename std::iterator_traits<It>::iterator_category,
         typename _Diff = typename std::iterator_traits<It>::difference_type>
class random_access_iterator_proxy : public bidirectional_iterator_proxy<It, V, PubV, _ItCat, _Diff>
{
    static_assert(std::is_base_of<std::random_access_iterator_tag, _ItCat>::value,
                  "random_access_iterator_proxy can only wrap random-access iterators");

    // Helper used to identify our type
    template<typename T> struct is_random_access_iterator_proxy : std::false_type { };
    template<typename OIt, typename OV, typename OPubV, typename _OItCat, typename _ODiff>
    struct is_random_access_iterator_proxy<random_access_iterator_proxy<OIt, OV, OPubV, _OItCat, _ODiff>> : std::true_type { };
public:
    random_access_iterator_proxy() = default;
    random_access_iterator_proxy(const random_access_iterator_proxy& other) = default;
    random_access_iterator_proxy(random_access_iterator_proxy&& other) = default;
    template<typename OIt, typename OV, typename OPubV, typename _OItCat, typename _ODiff>
    random_access_iterator_proxy(const random_access_iterator_proxy<OIt, OV, OPubV, _OItCat, _ODiff>& other)
        : bidirectional_iterator_proxy<It, V, PubV, _ItCat, _Diff>(static_cast<const bidirectional_iterator_proxy<OIt, OV, OPubV, _OItCat, _ODiff>&>(other)) { }
    template<typename OIt, typename OV, typename OPubV, typename _OItCat, typename _ODiff>
    random_access_iterator_proxy(random_access_iterator_proxy<OIt, OV, OPubV, _OItCat, _ODiff>&& other)
        : bidirectional_iterator_proxy<It, V, PubV, _ItCat, _Diff>(static_cast<bidirectional_iterator_proxy<OIt, OV, OPubV, _OItCat, _ODiff>&&>(other)) { }
    template<typename T,
             typename = std::enable_if_t<!is_random_access_iterator_proxy<std::decay_t<T>>::value, void>>
    random_access_iterator_proxy(T&& obj)
        : bidirectional_iterator_proxy<It, V, PubV, _ItCat, _Diff>(std::forward<T>(obj)) { }
    template<typename T1, typename T2, typename... Tx>
    random_access_iterator_proxy(T1&& obj1, T2&& obj2, Tx&&... objx)
        : bidirectional_iterator_proxy<It, V, PubV, _ItCat, _Diff>(std::forward<T1>(obj1), std::forward<T2>(obj2), std::forward<Tx>(objx)...) { }

    random_access_iterator_proxy& operator=(const random_access_iterator_proxy& other) = default;
    random_access_iterator_proxy& operator=(random_access_iterator_proxy&& other) = default;
    template<typename OIt, typename OV, typename OPubV, typename _OItCat, typename _ODiff>
    random_access_iterator_proxy& operator=(const random_access_iterator_proxy<OIt, OV, OPubV, _OItCat, _ODiff>& other) {
        bidirectional_iterator_proxy<It, V, PubV, _ItCat, _Diff>::operator=(other);
        return *this;
    }
    template<typename OIt, typename OV, typename OPubV, typename _OItCat, typename _ODiff>
    random_access_iterator_proxy& operator=(random_access_iterator_proxy<OIt, OV, OPubV, _OItCat, _ODiff>&& other) {
        bidirectional_iterator_proxy<It, V, PubV, _ItCat, _Diff>::operator=(std::move(other));
        return *this;
    }
    template<typename T,
             typename = std::enable_if_t<!is_random_access_iterator_proxy<std::decay_t<T>>::value, void>>
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

// Iterator proxy that wraps another iterator but converts the reference returned
// by operator*() to another reference type. Defined as follows:
//
// - If It is a pointer type, then iterator_proxy is PubV*.
// - Otherwise, if It is a random-access iterator, then iterator_proxy is random_access_iterator_proxy.
// - Otherwise, if It is a bidirectional iterator, then iterator_proxy is bidirectional_iterator_proxy.
// - Otherwise, if It is a forward iterator, then iterator_proxy is forward_iterator_proxy.
// - Otherwise, iterator_proxy is an incomplete type.
//
// Template arguments:
// It     - Type of iterator to proxy.
// V      - Type of values returned by It.
// PubV   - Type of values to publicly return references of. Must be either
//          V or a base class of V.
// _ItCat - Iterator category for It. Uses std::iterator_traits by default.
// _Diff  - Type used for differences between iterators. Uses
//          std::iterator_traits by default.
template<typename It,
         typename V,
         typename PubV,
         typename _ItCat = typename std::iterator_traits<It>::iterator_category,
         typename _Diff = typename std::iterator_traits<It>::difference_type>
using iterator_proxy = std::conditional_t<std::is_pointer<It>::value,
                                          PubV*,
                            std::conditional_t<std::is_base_of<std::random_access_iterator_tag, _ItCat>::value,
                                               random_access_iterator_proxy<It, V, PubV, _ItCat, _Diff>,
                                std::conditional_t<std::is_base_of<std::bidirectional_iterator_tag, _ItCat>::value,
                                                   bidirectional_iterator_proxy<It, V, PubV, _ItCat, _Diff>,
                                    std::conditional_t<std::is_base_of<std::forward_iterator_tag, _ItCat>::value,
                                                       forward_iterator_proxy<It, V, PubV, _ItCat, _Diff>,
                                                       incomplete_type>>>>;

// Iterator proxy that can wraps another iterator but convert the reference
// returned by operator*() to another reference type if needed. Defined as follows:
//
// - If PubV is the same type as V, then conditional_iterator_proxy is It.
// - Otherwise, conditional_iterator_proxy is iterator_proxy.
//
// Template arguments:
// It     - Type of iterator to proxy.
// V      - Type of values returned by It.
// PubV   - Type of values to publicly return references of. Must be either
//          V or a base class of V.
// _ItCat - Iterator category for It. Uses std::iterator_traits by default.
// _Diff  - Type used for differences between iterators. Uses
//          std::iterator_traits by default.
template<typename It,
         typename V,
         typename PubV,
         typename _ItCat = typename std::iterator_traits<It>::iterator_category,
         typename _Diff = typename std::iterator_traits<It>::difference_type>
using conditional_iterator_proxy = std::conditional_t<std::is_same<PubV, V>::value,
                                                      It,
                                                      iterator_proxy<It, V, PubV, _ItCat, _Diff>>;

// Helper type that calls sort_if_needed() on a lazy sorted container
// only if it does not support duplicates.
template<bool Multi> struct sort_lazy_container_if_needed_and_not_multi;
template<> struct sort_lazy_container_if_needed_and_not_multi<true> {
    template<typename LazyC> void operator()(LazyC&) const { }
};
template<> struct sort_lazy_container_if_needed_and_not_multi<false> {
    template<typename LazyC> void operator()(LazyC& c) const { c.sort_if_needed(); }
};

// Helper type that sorts the elements of a lazy sorted container.
// Implementation differs depending on whether containers accepts duplicates or not.
template<bool Multi> struct sort_lazy_container_elements;
template<> struct sort_lazy_container_elements<true> {
    template<typename LazyC> void operator()(LazyC& c) const {
        // For multi-value containers, we need to use std::stable_sort() because order
        // of equivalent elements must be preserved (since C++11).
        std::stable_sort(c.elements_.begin(), c.elements_.end(), c.vcmp_);
    }
};
template<> struct sort_lazy_container_elements<false> {
    template<typename LazyC> void operator()(LazyC& c) const {
        std::sort(c.elements_.begin(), c.elements_.end(), c.vcmp_);
        c.elements_.erase(std::unique(c.elements_.begin(), c.elements_.end(), c.veq_), c.elements_.end());
    }
};

// Helper type that updates a lazy sorted container's sorted_ flag after an
// insertion at the back of the container, depending on whether the insertion
// maintained the sorting or not. Use only on sorted containers with more
// than one element.
template<bool Multi> struct updated_lazy_container_sorted_flag_after_insert;
template<> struct updated_lazy_container_sorted_flag_after_insert<true> {
    template<typename LazyC> void operator()(LazyC& c) const {
        c.sorted_ = !c.vcmp_(c.elements_.back(), *(c.elements_.crbegin() + 1));
    }
};
template<> struct updated_lazy_container_sorted_flag_after_insert<false> {
    template<typename LazyC> void operator()(LazyC& c) const {
        c.sorted_ = c.vcmp_(*(c.elements_.crbegin() + 1), c.elements_.back());
    }
};

// Base type of lazy_sorted_container that includes a typedef mapped_type if T
// is not void, otherwise no typedef. Used to include the typedef for maps only.
template<typename T> struct mapped_type_base { typedef T mapped_type; };
template<> struct mapped_type_base<void> { };

// Implementation of a container that maintains a sorted set of elements, like
// std::set or std::map. The container uses another container internally to
// store elements in order to optimize size and speed; the container is sorted
// only when needed (or it can be triggered on-demand).
//
// This class meets the requirements of Container, ReversibleContainer,
// AllocatorAwareContainer and AssociativeContainer. Furthermore, the internal
// container type must meet the requirements of Container, ReversibleContainer,
// AllocatorAwareContainer and SequenceContainer. For details on those concepts,
// see http://en.cppreference.com/w/cpp/concept
//
// This class is not thread-safe; furthermore, because it performs on-demand
// sorting, there are only two ways of using it in a multi-threaded context:
// - Protect it with an exclusive mutex like std::mutex, or
// - Protect it with a shared mutex like std::shared_mutex and make sure that
//   it is always sorted before writer threads release their write locks.
//
// If this container is a map (e.g., type T is not void), this class will
// additionally have a mapped_type typedef (defined as T). Furthermore, if
// the map does not accept duplicates (e.g., Multi is false), the following
// additional methods are included:
// - at
// - operator[]
// - insert_or_assign
// - try_emplace
// Note, however, that those methods all require the container to be sorted to
// work, so they could be less efficient that blind inserts.
//
// Template arguments:
// K     - Type of keys used to sort elements in the container.
// T     - Type of values bound to each key. If this is set to void, the
//         container is assumed to be a set (like std::set) and its iterators
//         will always return const references. Otherwise, this is assumed to be
//         a map (like std::map) and will contain extra elements (see above).
// V     - Type of elements stored in the container.
// PubV  - Type of elements that container should appear to contain. Iterators
//         will return references to this type. Must either be the same as V
//         or a derived type (e.g., std::is_base_of<PubV, V>::value is true).
// VToK  - Predicate that, given a const V&, will return the key to use
//         to sort the element. For best performance, this should be a
//         const K& pointing to a key inside the element.
// KCmp  - Predicate used to compare keys.
// KEq   - Predicate used to test keys for equality.
// Alloc - Type of allocator to use for the underlying storage.
// Impl  - Type of container to use for internal storage. Must have the same
//         template parameters as std::vector/deque/etc.
// Multi - Whether container is allowed to have duplicates. Set this to true
//         for std::multiset/multimap-like containers.
template<typename K,
         typename T,
         typename V,
         typename PubV,
         typename VToK,
         typename KCmp,
         typename KEq,
         typename Alloc,
         template<typename _ImplT, typename _ImplAlloc> typename Impl,
         bool Multi,
         bool _IsNonMultiMap = !std::is_void<T>::value && !Multi>
class lazy_sorted_container : public mapped_type_base<T>
{
public:
    // Types for the internal container
    typedef Impl<V, Alloc>                                                                      container_impl;
    typedef typename container_impl::iterator                                                   iterator_impl;
    typedef typename container_impl::const_iterator                                             const_iterator_impl;
    typedef typename container_impl::reverse_iterator                                           reverse_iterator_impl;
    typedef typename container_impl::const_reverse_iterator                                     const_reverse_iterator_impl;
    
    // Types that are usually found in associative containers, like std::set/map
    typedef K                                                                                   key_type;
    typedef PubV                                                                                value_type;
    typedef typename container_impl::size_type                                                  size_type;
    typedef typename container_impl::difference_type                                            difference_type;
    typedef VToK                                                                                value_to_key;
    typedef KCmp                                                                                key_compare;
    typedef lazy_value_pred_proxy<value_type, value_to_key, key_compare>                        value_compare;
    typedef KEq                                                                                 key_equal_to;
    typedef lazy_value_pred_proxy<value_type, value_to_key, key_equal_to>                       value_equal_to;
    typedef typename container_impl::allocator_type                                             allocator_type;
    typedef std::conditional_t<std::is_void<T>::value, const value_type&, value_type&>          reference;
    typedef const value_type&                                                                   const_reference;
    typedef std::conditional_t<std::is_void<T>::value, const value_type*, value_type*>          pointer;
    typedef const value_type*                                                                   const_pointer;
    typedef std::conditional_t<std::is_void<T>::value,
                               conditional_iterator_proxy<const_iterator_impl, const V, const PubV>,
                               conditional_iterator_proxy<iterator_impl, V, PubV>>              iterator;
    typedef conditional_iterator_proxy<const_iterator_impl, const V, const PubV>                const_iterator;
    typedef std::conditional_t<std::is_void<T>::value,
                               conditional_iterator_proxy<const_reverse_iterator_impl, const V, const PubV>,
                               conditional_iterator_proxy<reverse_iterator_impl, V, PubV>>      reverse_iterator;
    typedef conditional_iterator_proxy<const_reverse_iterator_impl, const V, const PubV>        const_reverse_iterator;

private:
    mutable container_impl elements_;   // Container storing actual elements.
    mutable bool sorted_;               // Whether elements_ is currently sorted.
    value_to_key vtok_;                 // Predicate to get key for a given value.
    value_compare vcmp_;                // Value comparator.
    value_equal_to veq_;                // Value equality predictate.

private:
    // Friend some helper types.
    template<bool _HelperMulti> friend struct sort_lazy_container_if_needed_and_not_multi;
    template<bool _HelperMulti> friend struct sort_lazy_container_elements;
    template<bool _HelperMulti> friend struct updated_lazy_container_sorted_flag_after_insert;

public:
    // Default constructor. Constructs an empty container.
    lazy_sorted_container()
        : lazy_sorted_container(key_compare()) { }
    explicit lazy_sorted_container(const key_compare& kcmp,
                                   const allocator_type& alloc = allocator_type(),
                                   const key_equal_to& keq = key_equal_to())
        : elements_(alloc), sorted_(true),
          vtok_(), vcmp_(vtok_, kcmp),  veq_(vtok_, keq) { }
    explicit lazy_sorted_container(const allocator_type& alloc)
        : lazy_sorted_container(key_compare(), alloc) { }

    // Range constructor. Constructs a container with elements in the range [first, last[.
    template<typename It> lazy_sorted_container(It first,
                                                It last,
                                                const key_compare& kcmp = key_compare(),
                                                const allocator_type& alloc = allocator_type(),
                                                const key_equal_to& keq = key_equal_to())
        : elements_(first, last, alloc), sorted_(elements_.size() <= 1),
          vtok_(), vcmp_(vtok_, kcmp), veq_(vtok_, keq) { }
    template<typename It> lazy_sorted_container(It first,
                                                It last,
                                                const allocator_type& alloc)
        : lazy_sorted_container(first, last, key_compare(), alloc) { }

    // Copy constructor. Copies the content of another container, optionally replacing the allocator.
    lazy_sorted_container(const lazy_sorted_container& obj) = default;
    lazy_sorted_container(const lazy_sorted_container& obj, const allocator_type& alloc)
        : elements_(obj.elements_, alloc), sorted_(obj.sorted_),
          vtok_(obj.vtok_), vcmp_(obj.vcmp_), veq_(obj.veq_) { }

    // Move constructor. Moves the content of another container, optionally replacing the allocator.
    lazy_sorted_container(lazy_sorted_container&& obj)
        : elements_(std::move(obj.elements_)), sorted_(obj.sorted_),
          vtok_(std::move(obj.vtok_)), vcmp_(std::move(obj.vcmp_)), veq_(std::move(obj.veq_)) {
        obj.sorted_ = true;
    }
    lazy_sorted_container(lazy_sorted_container&& obj, const allocator_type& alloc)
        : elements_(std::move(obj.elements_), alloc), sorted_(obj.sorted_),
          vtok_(std::move(obj.vtok_)), vcmp_(std::move(obj.vcmp_)), veq_(std::move(obj.veq_)) {
        obj.sorted_ = true;
    }

    // Initializer-list constructor. Constructs a container with elements from an std::initializer_list.
    lazy_sorted_container(std::initializer_list<value_type> init,
                          const key_compare& kcmp = key_compare(),
                          const allocator_type& alloc = allocator_type(),
                          const key_equal_to& keq = key_equal_to())
        : lazy_sorted_container(std::begin(init), std::end(init), kcmp, alloc, keq) { }
    lazy_sorted_container(std::initializer_list<value_type> init,
                          const allocator_type& alloc)
        : lazy_sorted_container(std::begin(init), std::end(init), alloc) { }

    // Assignment operators. Copies or moves content from another container or an std::initializer_list.
    lazy_sorted_container& operator=(const lazy_sorted_container& obj) = default;
    lazy_sorted_container& operator=(lazy_sorted_container&& obj) {
        elements_ = std::move(obj.elements_);
        sorted_ = obj.sorted_;
        vtok_ = std::move(obj.vtok_);
        vcmp_ = std::move(obj.vcmp_);
        veq_ = std::move(obj.veq_);
        obj.sorted_ = true;
        return *this;
    }
    lazy_sorted_container& operator=(std::initializer_list<value_type> init) {
        elements_.clear();
        elements_.insert(elements_.cend(), std::begin(init), std::end(init));
        sorted_ = elements_.size() <= 1;
        return *this;
    }

    // Equality operators. Validates that both containers have the same elements.
    // Note: does not use key_equal_to to compare elements; elements must be EqualityComparable for this to work.
    // For more information, see http://en.cppreference.com/w/cpp/concept/EqualityComparable
    friend bool operator==(const lazy_sorted_container& left, const lazy_sorted_container& right) {
        // We need both containers to be sorted for this to work.
        left.sort_if_needed();
        right.sort_if_needed();
        return std::equal(left.elements_.cbegin(), left.elements_.cend(),
                          right.elements_.cbegin(), right.elements_.cend());
    }
    friend bool operator!=(const lazy_sorted_container& left, const lazy_sorted_container& right) {
        return !(left == right);
    }

    // Comparison operators. Performs a lexicographical comparison of two containers.
    // Note: does not use key_compare to compare elements; elements must be LessThanComparable for this to work.
    // For more information, see http://en.cppreference.com/w/cpp/concept/LessThanComparable
    friend bool operator<(const lazy_sorted_container& left, const lazy_sorted_container& right) {
        // We need both containers to be sorted for this to work.
        left.sort_if_needed();
        right.sort_if_needed();
        return std::lexicographical_compare(left.elements_.cbegin(), left.elements_.cend(),
                                            right.elements_.cbegin(), right.elements_.cend());
    }
    friend bool operator<=(const lazy_sorted_container& left, const lazy_sorted_container& right) {
        return !(right < left);
    }
    friend bool operator>(const lazy_sorted_container& left, const lazy_sorted_container& right) {
        return right < left;
    }
    friend bool operator>=(const lazy_sorted_container& left, const lazy_sorted_container& right) {
        return !(left < right);
    }

    // Element access (only available for non-multi maps).
    template<typename _T = T,
             typename _TRef = std::enable_if_t<_IsNonMultiMap, _T&>>
    _TRef at(const key_type& key) {
        sort_if_needed();
        auto elem_end = elements_.end();
        auto it = std::lower_bound(elements_.begin(), elem_end, key, vcmp_);
        if (it == elem_end || vcmp_(key, *it)) {
            throw_out_of_range();
        }
        return it->second;
    }
    template<typename _T = T,
             typename _CTRef = std::enable_if_t<_IsNonMultiMap, const _T&>>
    _CTRef at(const key_type& key) const {
        sort_if_needed();
        auto elem_cend = elements_.cend();
        auto cit = std::lower_bound(elements_.cbegin(), elem_cend, key, vcmp_);
        if (cit == elem_cend || vcmp_(key, *cit)) {
            throw_out_of_range();
        }
        return cit->second;
    }
    template<typename _T = T,
             typename _TRef = std::enable_if_t<_IsNonMultiMap, _T&>>
    _TRef operator[](const key_type& key) {
        return operator_brackets_impl(key);
    }
    template<typename _T = T,
             typename _TRef = std::enable_if_t<_IsNonMultiMap, _T&>>
    _TRef operator[](key_type&& key) {
        return operator_brackets_impl(std::move(key));
    }

    // Methods to obtain iterators.
    iterator begin() {
        // Container always needs to be sorted when we iterate.
        sort_if_needed();
        // Note that this works even if iterator type is a proxy, because it has
        // an implicit converting constructor from the proxied iterator type.
        return elements_.begin();
    }
    const_iterator begin() const {
        return cbegin();
    }
    const_iterator cbegin() const {
        sort_if_needed();
        return elements_.cbegin();
    }
    iterator end() {
        sort_if_needed();
        return elements_.end();
    }
    const_iterator end() const {
        return cend();
    }
    const_iterator cend() const {
        sort_if_needed();
        return elements_.cend();
    }

    reverse_iterator rbegin() {
        sort_if_needed();
        return elements_.rbegin();
    }
    const_reverse_iterator rbegin() const {
        return crbegin();
    }
    const_reverse_iterator crbegin() const {
        sort_if_needed();
        return elements_.crbegin();
    }
    reverse_iterator rend() {
        sort_if_needed();
        return elements_.rend();
    }
    const_reverse_iterator rend() const {
        return crend();
    }
    const_reverse_iterator crend() const {
        sort_if_needed();
        return elements_.crend();
    }

    // Size / capacity
    bool empty() const {
        return elements_.empty();
    }
    size_type size() const {
        // If we're not sorted and this container does not accept duplicates, we have no choice but to sort.
        sort_lazy_container_if_needed_and_not_multi<Multi>()(*this);
        return elements_.size();
    }
    size_type max_size() const {
        return elements_.max_size();
    }

    // Optional methods pertaining to capacity
    // Only available if the internal container supports them.
    template<typename = std::enable_if_t<has_reserve_method<container_impl, size_type>::value, void>>
    void reserve(size_type new_cap) {
        elements_.reserve(new_cap);
    }
    template<typename = std::enable_if_t<has_capacity_const_method<container_impl>::value, void>>
    auto capacity() const {
        return elements_.capacity();
    }
    template<typename = std::enable_if_t<has_shrink_to_fit_method<container_impl>::value, void>>
    void shrink_to_fit() {
        elements_.shrink_to_fit();
    }

    // Modifiers
    // Note.1: because of lazy sorting, methods can't return iterators; they return void instead.
    // Note.2: for containers that do not accept duplicates, inserted duplicates are silently stripped when sorted.
    void insert(const value_type& value) {
        // What we do is push new value in the internal container, then check if container is still sorted.
        elements_.push_back(value);
        if (sorted_) {
            update_sorted_after_push_back();
        }
    }
    void insert(value_type&& value) {
        elements_.push_back(std::move(value));
        if (sorted_) {
            update_sorted_after_push_back();
        }
    }
    void insert(const_iterator, const value_type& value) {
        // Ignore hint iterator in favor of lazy sorting
        insert(value);
    }
    void insert(const_iterator, value_type&& value) {
        insert(std::move(value));
    }
    template<typename It> void insert(It first, It last) {
        // Checking if container is sorted would be onerous for batch inserts.
        elements_.insert(elements_.cend(), first, last);
        sorted_ = elements_.size() <= 1;
    }
    void insert(std::initializer_list<value_type> init) {
        elements_.insert(elements_.cend(), std::begin(init), std::end(init));
        sorted_ = elements_.size() <= 1;
    }
    template<typename... Args> void emplace(Args&&... args) {
        elements_.emplace_back(std::forward<Args>(args)...);
        if (sorted_) {
            update_sorted_after_push_back();
        }
    }
    template<typename... Args> void emplace_hint(const_iterator, Args&&... args) {
        // Ignore hint iterator in favor of lazy sorting
        emplace(std::forward<Args>(args)...);
    }
    iterator erase(const_iterator pos) {
        // If user has a valid iterator, it's because container is sorted.
        return elements_.erase(pos);
    }
    iterator erase(const_iterator first, const_iterator last) {
        return elements_.erase(first, last);
    }
    size_type erase(const key_type& key) {
        auto range = equal_range(key);
        size_type dist = std::distance(range.first, range.second);
        erase(range.first, range.second);
        return dist;
    }
    void clear() {
        elements_.clear();
        sorted_ = true;
    }
    void swap(lazy_sorted_container& obj) {
        using std::swap;
        swap(elements_, obj.elements_);
        swap(sorted_, obj.sorted_);
        swap(vtok_, obj.vtok_);
        swap(vcmp_, obj.vcmp_);
        swap(veq_, obj.veq_);
    }
    // Non-member swap that can be found via ADL
    friend void swap(lazy_sorted_container& obj1, lazy_sorted_container& obj2) {
        obj1.swap(obj2);
    }

    // Modifiers that are only available for non-multi maps
    template<typename OT,
             bool _Enabled = _IsNonMultiMap,
             typename = std::enable_if_t<_Enabled, void>>
    std::pair<iterator, bool> insert_or_assign(const key_type& key, OT&& val) {
        return insert_or_assign_impl(key, std::forward<OT>(val));
    }
    template<typename OT,
             bool _Enabled = _IsNonMultiMap,
             typename = std::enable_if_t<_Enabled, void>>
    std::pair<iterator, bool> insert_or_assign(key_type&& key, OT&& val) {
        return insert_or_assign_impl(std::move(key), std::forward<OT>(val));
    }
    template<typename... Args,
             bool _Enabled = _IsNonMultiMap,
             typename = std::enable_if_t<_Enabled, void>>
    std::pair<iterator, bool> try_emplace(const key_type& key, Args&&... args) {
        return try_emplace_impl(key, std::forward<Args>(args)...);
    }
    template<typename... Args,
             bool _Enabled = _IsNonMultiMap,
             typename = std::enable_if_t<_Enabled, void>>
    std::pair<iterator, bool> try_emplace(key_type&& key, Args&&... args) {
        return try_emplace_impl(std::move(key), std::forward<Args>(args)...);
    }

    // Lookups
    size_type count(const key_type& key) const {
        auto range = equal_range(key);
        return std::distance(range.first, range.second);
    }
    iterator find(const key_type& key) {
        sort_if_needed();
        auto elem_end = elements_.end();
        auto it = std::lower_bound(elements_.begin(), elem_end, key, vcmp_);
        return (it != elem_end && !vcmp_(key, *it)) ? it : elem_end;
    }
    const_iterator find(const key_type& key) const {
        sort_if_needed();
        auto elem_cend = elements_.cend();
        auto cit = std::lower_bound(elements_.cbegin(), elem_cend, key, vcmp_);
        return (cit != elem_cend && !vcmp_(key, *cit)) ? cit : elem_cend;
    }
    iterator lower_bound(const key_type& key) {
        sort_if_needed();
        return std::lower_bound(elements_.begin(), elements_.end(), key, vcmp_);
    }
    const_iterator lower_bound(const key_type& key) const {
        sort_if_needed();
        return std::lower_bound(elements_.cbegin(), elements_.cend(), key, vcmp_);
    }
    iterator upper_bound(const key_type& key) {
        sort_if_needed();
        return std::upper_bound(elements_.begin(), elements_.end(), key, vcmp_);
    }
    const_iterator upper_bound(const key_type& key) const {
        sort_if_needed();
        return std::upper_bound(elements_.cbegin(), elements_.cend(), key, vcmp_);
    }
    std::pair<iterator, iterator> equal_range(const key_type& key) {
        sort_if_needed();
        return std::equal_range(elements_.begin(), elements_.end(), key, vcmp_);
    }
    std::pair<const_iterator, const_iterator> equal_range(const key_type& key) const {
        sort_if_needed();
        return std::equal_range(elements_.cbegin(), elements_.cend(), key, vcmp_);
    }

    // Lookups with any type supported by key comparator
    // Only available if key comparator is a transparent function
    // (e.g. defines is_transparent), like std::less<>.
    template<typename OK,
             typename _OKCmp = key_compare,
             typename = typename _OKCmp::is_transparent>
    size_type count(const OK& key) const {
        auto range = equal_range(key);
        return std::distance(range.first, range.second);
    }
    template<typename OK,
             typename _OKCmp = key_compare,
             typename = typename _OKCmp::is_transparent>
    iterator find(const OK& key) {
        sort_if_needed();
        auto elem_end = elements_.end();
        auto it = std::lower_bound(elements_.begin(), elem_end, key, vcmp_);
        return (it != elem_end && !vcmp_(key, *it)) ? it : elem_end;
    }
    template<typename OK,
             typename _OKCmp = key_compare,
             typename = typename _OKCmp::is_transparent>
    const_iterator find(const OK& key) const {
        sort_if_needed();
        auto elem_cend = elements_.cend();
        auto cit = std::lower_bound(elements_.cbegin(), elem_cend, key, vcmp_);
        return (cit != elem_cend && !vcmp_(key, *cit)) ? cit : elem_cend;
    }
    template<typename OK,
             typename _OKCmp = key_compare,
             typename = typename _OKCmp::is_transparent>
    iterator lower_bound(const OK& key) {
        sort_if_needed();
        return std::lower_bound(elements_.begin(), elements_.end(), key, vcmp_);
    }
    template<typename OK,
             typename _OKCmp = key_compare,
             typename = typename _OKCmp::is_transparent>
    const_iterator lower_bound(const OK& key) const {
        sort_if_needed();
        return std::lower_bound(elements_.cbegin(), elements_.cend(), key, vcmp_);
    }
    template<typename OK,
             typename _OKCmp = key_compare,
             typename = typename _OKCmp::is_transparent>
    iterator upper_bound(const OK& key) {
        sort_if_needed();
        return std::upper_bound(elements_.begin(), elements_.end(), key, vcmp_);
    }
    template<typename OK,
             typename _OKCmp = key_compare,
             typename = typename _OKCmp::is_transparent>
    const_iterator upper_bound(const OK& key) const {
        sort_if_needed();
        return std::upper_bound(elements_.cbegin(), elements_.cend(), key, vcmp_);
    }
    template<typename OK,
             typename _OKCmp = key_compare,
             typename = typename _OKCmp::is_transparent>
    std::pair<iterator, iterator> equal_range(const OK& key) {
        sort_if_needed();
        return std::equal_range(elements_.begin(), elements_.end(), key, vcmp_);
    }
    template<typename OK,
             typename _OKCmp = key_compare,
             typename = typename _OKCmp::is_transparent>
    std::pair<const_iterator, const_iterator> equal_range(const OK& key) const {
        sort_if_needed();
        return std::equal_range(elements_.cbegin(), elements_.cend(), key, vcmp_);
    }

    // Predicates / allocator
    key_compare key_comp() const {
        return vcmp_.key_predicate();
    }
    value_compare value_comp() const {
        return vcmp_;
    }
    allocator_type get_allocator() const {
        return elements_.get_allocator();
    }
    key_equal_to key_eq() const {
        return veq_.key_predicate();
    }
    value_equal_to value_eq() const {
        return veq_;
    }

    // Sorting
    bool sorted() const {
        return sorted_;
    }
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
    template<typename OK,
             typename _T = T,
             typename _TRef = std::enable_if_t<_IsNonMultiMap && std::is_same<std::decay_t<OK>, key_type>::value, _T&>>
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
    template<typename OK,
             typename OT,
             typename = std::enable_if_t<_IsNonMultiMap && std::is_same<std::decay_t<OK>, key_type>::value, void>>
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
    template<typename OK,
             typename... Args,
             typename = std::enable_if_t<_IsNonMultiMap && std::is_same<std::decay_t<OK>, key_type>::value, void>>
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

// Predicate that implements "equal to" using a comparator like std::less.
template<typename T,
         typename _Cmp = std::less<T>>
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

// Proxy for std::equal_to that has a constructor that accepts an arbitrary
// unused predicate. Used to bridge with equal_to_using_less.
template<typename T,
         typename _UnusedCmp = std::less<T>>
struct equal_to_proxy : std::equal_to<T> {
    equal_to_proxy() = default;
    explicit equal_to_proxy(const _UnusedCmp&) : equal_to_proxy() { }
};

// "equal to" predicate that uses operator==() if it's available for the type,
// otherwise implements it using equal_to_using_less.
template<typename T,
         typename _Cmp = std::less<T>>
using equal_to_using_less_if_needed = std::conditional_t<has_operator_equal<T>::value,
                                                         equal_to_proxy<T, _Cmp>,
                                                         equal_to_using_less<T, _Cmp>>;

// Unary predicate that projects a value reference unmodified.
// Used as "value "to key" for set-like containers.
template<typename T>
struct identity {
    const T& operator()(const T& obj) const {
        return obj;
    }
};

// Unary predicate that projects the reference to a pair's first element.
// Used as "value to key" for map-like containers.
template<typename P>
struct pair_first {
    decltype(auto) operator()(const P& obj) const {
        return std::get<0>(obj);
    }
};

// Specialization of std::pair that supports assigning to the first element
// even if it is const. This will be used to build lazy std::map-like
// containers that can copy/move their elements in their internal container.
//
// The pair's first element is implicitely const. Use like this:
//
//    detail::map_pair<int, float>  // inherits from std::pair<const int, float>
template<typename RT1,
         typename T2,
         typename _BaseStdPair = std::pair<const RT1, T2>>
class map_pair : public _BaseStdPair
{
public:
    // Typedef that can be used to refer to our base std::pair class.
    typedef _BaseStdPair base_std_pair;

private:
    // Helper constructor called by the piecewise constructor.
    // This trick is used to be able to get an integer_sequence
    // representing the indexes of arguments in the parameter pack.
    template<typename Args1, typename Args2, size_t... Indexes1, size_t... Indexes2>
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
    template<typename U1, typename U2>
    map_pair(U1&& fir, U2&& sec)
        : base_std_pair(std::forward<U1>(fir), std::forward<U2>(sec)) { }

    // Copy and move constructors.
    map_pair(const map_pair&) = default;
    map_pair(map_pair&& obj)
        : base_std_pair(std::move(const_cast<RT1&>(obj.first)), std::move(obj.second)) { }

    // Constructors from compatible map_pair's.
    template<typename RU1, typename U2>
    map_pair(const map_pair<RU1, U2>& obj) : base_std_pair(obj) { }
    template<typename RU1, typename U2>
    map_pair(map_pair<RU1, U2>&& obj)
        : base_std_pair(std::move(const_cast<RU1&>(obj.first)), std::move(obj.second)) { }

    // Constructors from std::pair.
    template<typename U1, typename U2>
    map_pair(const std::pair<U1, U2>& obj) : base_std_pair(obj) { }
    template<typename U1, typename U2>
    map_pair(std::pair<U1, U2>&& obj)
        : base_std_pair(std::move(const_cast<std::remove_reference_t<U1>&>(obj.first)), std::move(obj.second)) { }

    // Piecewise constructor. Calls the private helper constructor (see above)
    // to avoid copying/moving objects in the tuple one too many time.
    template<typename... Args1, typename... Args2>
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
    template<typename RU1, typename U2>
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
    template<typename RU1, typename U2>
    map_pair& operator=(map_pair<RU1, U2>&& obj) {
        const_cast<RT1&>(this->first) = std::move(const_cast<RU1&>(obj.first));
        this->second = std::move(obj.second);
        return *this;
    }

    // Assignment operator from std::pair.
    template<typename U1, typename U2>
    map_pair& operator=(const std::pair<U1, U2>& obj) {
        const_cast<RT1&>(this->first) = obj.first;
        this->second = obj.second;
        return *this;
    }

    // Move assignment operator from std::pair.
    template<typename U1, typename U2>
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

// Default allocator type for lazy maps that use map_pair. Will be brought
// into the coveo::lazy namespace in the map.h header. Can be used to declare
// lazy map classes with custom allocators without having to reference
// the internal map_pair class:
//
//   // my_custom_allocator must be a template accepting a single type argument
//   coveo::lazy::map_allocator<key_type, mapped_type, my_custom_allocator>
//
// By default, the allocator type is std::allocator.
template<typename K,
         typename T,
         template<typename _AllocT> typename _Alloc = std::allocator>
using map_allocator = _Alloc<map_pair<K, T>>;

} // detail
} // lazy
} // coveo

#endif // COVEO_LAZY_DETAIL_LAZY_SORTED_CONTAINER_H
/**
 * @file
 * @brief Iterator helpers for lazy-sorted containers.
 *
 * @copyright 2015-2016, Coveo Solutions Inc.
 *            Distributed under the Apache License, Version 2.0 (see <a href="https://github.com/coveo/lazy/blob/master/LICENSE">LICENSE</a>).
 */

#ifndef COVEO_LAZY_ITERATOR_H
#define COVEO_LAZY_ITERATOR_H

#include <iterator>
#include <memory>
#include <utility>

namespace coveo {
namespace lazy {

/**
 * @brief Insertion iterator for lazy-sorted containers.
 * @headerfile iterator.h <coveo/lazy/iterator.h>
 *
 * Equivalent of <tt>std::insert_iterator</tt> usable with lazy
 * sorted containers. Always performs blind insertions.
 * Mirrors http://en.cppreference.com/w/cpp/iterator/insert_iterator
 *
 * @see coveo::lazy::inserter()
 */
template<typename Container>
class insert_iterator
{
public:
    // Standard iterator typedefs
    using iterator_category = std::output_iterator_tag;
    using value_type = void;
    using difference_type = void;
    using pointer = void;
    using reference = void;

    // Type of container we insert into
    using container_type = Container;

    // Constructor
    explicit insert_iterator(container_type& c)
        : pc_(std::addressof(c)) { }

    // Assignment operators that copy/move values in the container
    insert_iterator& operator=(const typename container_type::value_type& value) {
        pc_->insert(value);
        return *this;
    }
    insert_iterator& operator=(typename container_type::value_type&& value) {
        pc_->insert(std::move(value));
        return *this;
    }

    // No-op required to meet the requirements of OutputIterator
    insert_iterator& operator*() { return *this; }
    insert_iterator& operator++() { return *this; }
    insert_iterator& operator++(int) { return *this; }

    /// @cond NEVERSHOWN

protected:
    container_type* pc_;    // Pointer to container where to insert

    /// @endcond
};

/**
 * @brief Helper function to create <tt>coveo::lazy::insert_iterator</tt> instances.
 * @headerfile iterator.h <coveo/lazy/iterator.h>
 *
 * Creates an <tt>coveo::lazy::insert_iterator</tt> instance that
 * will perform blind insertions in container @c c.
 *
 * @see coveo::lazy::insert_iterator
 */
template<typename Container> auto inserter(Container& c) {
    return insert_iterator<Container>(c);
}

} // namespace lazy
} // namespace coveo

#endif // COVEO_LAZY_ITERATOR_H

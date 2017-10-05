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
    /**
     * @brief Iterator category.
     *
     * Typedef representing the type of iterator. In our case, this is
     * defined as <tt>std::output_iterator_tag</tt> to indicate this is
     * an output iterator.
     *
     * Part of the five standard iterator typedefs that guarantee that
     * <tt>std::iterator_traits</tt> work for our type.
     */
    using iterator_category = std::output_iterator_tag;

    /**
     * @brief Type of value returned by iterator.
     *
     * Typedef representing the type of value returned by the iterator.
     * In our case, since this is an output iterator, it is defined as @c void.
     *
     * Part of the five standard iterator typedefs that guarantee that
     * <tt>std::iterator_traits</tt> work for our type.
     */
    using value_type = void;

    /**
     * @brief Difference between iterators.
     *
     * Typedef representing the difference between two iterators.
     * In our case, since this is an output iterator, it is defined as @c void.
     *
     * Part of the five standard iterator typedefs that guarantee that
     * <tt>std::iterator_traits</tt> work for our type.
     */
    using difference_type = void;

    /**
     * @brief Value pointer.
     *
     * Typedef representing the pointer to <tt>insert_iterator::value_type</tt>.
     * In our case, since this is an output iterator, it is defined as @c void.
     *
     * Part of the five standard iterator typedefs that guarantee that
     * <tt>std::iterator_traits</tt> work for our type.
     */
    using pointer = void;

    /**
     * @brief Value reference.
     *
     * Typedef representing the reference to <tt>insert_iterator::value_type</tt>.
     * In our case, since this is an output iterator, it is defined as @c void.
     *
     * Part of the five standard iterator typedefs that guarantee that
     * <tt>std::iterator_traits</tt> work for our type.
     */
    using reference = void;

    /**
     * @brief Type of container we insert into.
     *
     * Type of container that this iterator inserts into.
     */
    using container_type = Container;

    /**
     * @brief Constructor.
     *
     * Constructor. Creates an output iterator that will perform
     * blind insertions in container @c c.
     *
     * @param c Container to insert into.
     */
    explicit insert_iterator(container_type& c)
        : pc_(std::addressof(c)) { }

    /**
     * @brief Inserts element in the container.
     *
     * Inserts an element (by copying it) in the container that
     * was passed to the constructor. Always performs a blind insertion.
     *
     * @param value Element to insert.
     * @return Reference to @c this output iterator.
     */
    insert_iterator& operator=(const typename container_type::value_type& value) {
        pc_->insert(value);
        return *this;
    }

    /**
     * @brief Inserts element in the container (move version).
     *
     * Inserts an element (by moving it) in the container that
     * was passed to the constructor. Always performs a blind insertion.
     *
     * @param value Element to insert (by moving it).
     * @return Reference to @c this output iterator.
     */
    insert_iterator& operator=(typename container_type::value_type&& value) {
        pc_->insert(std::move(value));
        return *this;
    }

    /**
     * @brief No-op.
     *
     * Operator provided to conform with the @c OutputIterator
     * concept. A no-op that simply returns a reference to @c this.
     *
     * @return Reference to @c this output iterator
     */
    insert_iterator& operator*() { return *this; }

    /**
     * @brief No-op.
     *
     * Operator provided to conform with the @c OutputIterator
     * concept. A no-op that simply returns a reference to @c this.
     *
     * @return Reference to @c this output iterator
     */
    insert_iterator& operator++() { return *this; }

    /**
     * @brief No-op.
     *
     * Operator provided to conform with the @c OutputIterator
     * concept. A no-op that simply returns a reference to @c this.
     *
     * @return Reference to @c this output iterator
     */
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

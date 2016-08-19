// Copyright (c) 2015-2016, Coveo Solutions Inc.
// Distributed under the Apache License, Version 2.0 (see LICENSE).

// Helper iterators to be used with lazy sorted containers.

#ifndef COVEO_LAZY_ITERATOR_H
#define COVEO_LAZY_ITERATOR_H

#include <iterator>
#include <memory>
#include <utility>

namespace coveo {
namespace lazy {

// Equivalent of std::insert_iterator usable with lazy sorted containers.
// Always performs blind insertions.
// Mirrors http://en.cppreference.com/w/cpp/iterator/insert_iterator
template<typename Container>
class insert_iterator : public std::iterator<std::output_iterator_tag, void, void, void, void>
{
public:
    // Type of container we insert into
    typedef Container container_type;

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

protected:
    container_type* pc_;    // Pointer to container where to insert
};

// Helper functions to create coveo::lazy::insert_iterator instances
template<typename Container> auto inserter(Container& c) {
    return insert_iterator<Container>(c);
}

} // namespace lazy
} // namespace coveo

#endif // COVEO_LAZY_ITERATOR_H

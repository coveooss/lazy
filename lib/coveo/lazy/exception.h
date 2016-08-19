// Copyright (c) 2015-2016, Coveo Solutions Inc.
// Distributed under the Apache License, Version 2.0 (see LICENSE).

// Definition of exceptions used in the lazy sorted container library.

#ifndef COVEO_LAZY_EXCEPTION_H
#define COVEO_LAZY_EXCEPTION_H

#include <stdexcept>

namespace coveo {
namespace lazy {

// Subclass of std::out_of_range used by lazy sorted containers.
// Thrown by methods like coveo::lazy::map::at().
// For mor information, see http://en.cppreference.com/w/cpp/error/out_of_range
class out_of_range : public std::out_of_range
{
public:
    out_of_range() = delete;
    using std::out_of_range::out_of_range;
};

} // lazy
} // coveo

#endif // COVEO_LAZY_EXCEPTION_H

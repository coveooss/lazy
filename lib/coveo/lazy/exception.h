/**
 * @file
 * @brief Exception classes used by lazy-sorted associative containers.
 *
 * @copyright 2015-2016, Coveo Solutions Inc.
 *            Distributed under the Apache License, Version 2.0 (see <a href="https://github.com/coveo/lazy/blob/master/LICENSE">LICENSE</a>).
 */

#ifndef COVEO_LAZY_EXCEPTION_H
#define COVEO_LAZY_EXCEPTION_H

#include <stdexcept>

namespace coveo {
namespace lazy {

/**
 * @brief Out-of-range exception.
 * @headerfile exception.h <coveo/lazy/exception.h>
 *
 * Subclass of <tt>std::out_of_range</tt> used by lazy sorted containers.
 * Thrown by methods like <tt>coveo::lazy::map::at()</tt>.
 * For mor information, see http://en.cppreference.com/w/cpp/error/out_of_range
 */
class out_of_range : public std::out_of_range
{
public:
    out_of_range() = delete;
    using std::out_of_range::out_of_range;
};

} // lazy
} // coveo

#endif // COVEO_LAZY_EXCEPTION_H

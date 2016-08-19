// Copyright (c) 2015-2016, Coveo Solutions Inc.
// Distributed under the Apache License, Version 2.0 (see LICENSE).

#include "coveo/lazy/all_tests.h"

#include "coveo/lazy/map_tests.h"
#include "coveo/lazy/set_tests.h"

namespace coveo_tests {
namespace lazy {

// Runs all tests for coveo::lazy classes
void all_tests()
{
    // map/multimap
    map_tests();
    multimap_tests();

    // set/multiset
    set_tests();
    multiset_tests();
}

// Runs all benchmarks for coveo::lazy classes
void all_benchmarks()
{
    set_benchmarks();
}

} // lazy
} // coveo_tests

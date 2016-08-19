// Copyright (c) 2015-2016, Coveo Solutions Inc.
// Distributed under the Apache License, Version 2.0 (see LICENSE).

#include <coveo/lazy/all_tests.h>
#include <coveo/test_framework.h>

#include <iostream>
#include <string>

// Test program entry point.
// Runs all tests or all benchmarks depending on defines.
int main()
{
    int ret = 0;

#ifndef COVEO_LAZY_BENCHMARKS
    std::cout << "Running tests..." << std::endl;
    ret = coveo_tests::run_tests(&coveo_tests::lazy::all_tests);
#else
    std::cout << "Running benchmarks..." << std::endl;
    ret = coveo_tests::run_tests(&coveo_tests::lazy::all_benchmarks);
#endif
    std::cout << "Done." << std::endl;

    if (ret != 0) {
        std::cout << std::endl << "Press enter to continue ";
        std::string unused;
        std::getline(std::cin, unused);
    }
    
    return ret;
}

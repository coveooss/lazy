// Copyright (c) 2015-2016, Coveo Solutions Inc.
// Distributed under the Apache License, Version 2.0 (see LICENSE).

#include "coveo/lazy/set_tests.h"

#include <coveo/lazy/iterator.h>
#include <coveo/lazy/set.h>
#include <coveo/test_framework.h>

#include <algorithm>
#include <chrono>
#include <functional>
#include <iostream>
#include <initializer_list>
#include <iterator>
#include <random>
#include <set>
#include <string>
#include <typeinfo>
#include <vector>
#include <utility>

namespace coveo_tests {
namespace lazy {
namespace detail {

// Helpers to compare containers without using native operators.
template<typename C1, typename C2>
bool containers_are_equal(const C1& c1, const C2& c2) {
    return std::equal(std::cbegin(c1), std::cend(c1), std::cbegin(c2), std::cend(c2));
}
template<typename C1, typename C2>
bool container_is_less_than(const C1& c1, const C2& c2) {
    return std::lexicographical_compare(std::cbegin(c1), std::cend(c1), std::cbegin(c2), std::cend(c2));
}

// Simple class that can be compared with ints.
class Int
{
    int val_;
public:
    Int(int val = 0) : val_(val) { }
    int value() const { return val_; }

    friend bool operator<(const Int& left, const Int& right) {
        return left.val_ < right.val_;
    }
    friend bool operator<(const Int& left, int right) {
        return left.val_ < right;
    }
    friend bool operator<(int left, const Int& right) {
        return left < right.val_;
    }
};

// Performs a benchmark that inserts multiple elements in two sets,
// then performs some set operations.
template<typename SetT>
void benchmark_inserts_and_set_operations(const char* const test_name)
{
    typedef SetT set_type;

    // Generate random identifiers
    std::mt19937_64 rand;
    std::uniform_int_distribution<size_t> dist_all;
    const size_t NUM_IDS = 10000000;
    std::vector<size_t> ids;
    ids.resize(NUM_IDS);
    std::generate_n(ids.begin(), NUM_IDS, std::bind(dist_all, std::ref(rand)));
    std::uniform_int_distribution<size_t> dist_ids(0, ids.size() - 1);

    std::cout << "Starting benchmark_inserts_and_set_operations for " << test_name << "..." << std::endl;
    auto start_marker = std::chrono::steady_clock::now();

    {
        // Create two sets for testing and one resulting set
        set_type s1, s2, sres;

        auto test_run = [&]() {
            // Add random identifiers to each test set
            const size_t NUM_IDS_IN_SETS = 1000000;
            auto gen_set = [&](set_type& s) {
                std::generate_n(coveo::lazy::inserter(s), NUM_IDS_IN_SETS,
                                std::bind(dist_ids, std::ref(rand)));
            };
            gen_set(s1);
            gen_set(s2);

            // Perform some set operations
            {
                sres.clear();
                std::merge(s1.cbegin(), s1.cend(), s2.cbegin(), s2.cend(),
                           coveo::lazy::inserter(sres), s1.value_comp());
            }
            {
                sres.clear();
                std::set_difference(s1.cbegin(), s1.cend(), s2.cbegin(), s2.cend(),
                                    coveo::lazy::inserter(sres), s1.value_comp());
            }
            {
                sres.clear();
                std::set_intersection(s1.cbegin(), s1.cend(), s2.cbegin(), s2.cend(),
                                      coveo::lazy::inserter(sres), s1.value_comp());
            }
            {
                sres.clear();
                std::set_symmetric_difference(s1.cbegin(), s1.cend(), s2.cbegin(), s2.cend(),
                                              coveo::lazy::inserter(sres), s1.value_comp());
            }
            {
                sres.clear();
                std::set_union(s1.cbegin(), s1.cend(), s2.cbegin(), s2.cend(),
                               coveo::lazy::inserter(sres), s1.value_comp());
            }
        };

        // Run test a number of times
        const size_t NUM_TEST_RUNS = 3;
        for (size_t i = 0; i < NUM_TEST_RUNS; ++i) {
            test_run();
        }
    }

    auto end_marker = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_s = end_marker - start_marker;
    std::cout << "Benchmark completed in " << elapsed_s.count() << "s" << std::endl;
}

// Performs a benchmark that mixes insertions and lookups in a set.
template<typename SetT>
void benchmark_mixed_set_operations(const char* const test_name)
{
    typedef SetT set_type;

    // Generate random identifiers
    std::mt19937_64 rand;
    std::uniform_int_distribution<size_t> dist_all;
    const size_t NUM_IDS = 100000;
    std::vector<size_t> ids;
    ids.resize(NUM_IDS);
    std::generate_n(ids.begin(), NUM_IDS, std::bind(dist_all, std::ref(rand)));
    std::uniform_int_distribution<size_t> dist_ids(0, ids.size() - 1);

    std::cout << "Starting benchmark_mixed_set_operations for " << test_name << "..." << std::endl;
    auto start_marker = std::chrono::steady_clock::now();

    {
        // Create a set and perform random operations
        set_type s;
        const size_t NUM_OPERATIONS = 100000;
        size_t num_found = 0;
        std::uniform_int_distribution<size_t> dist_bool(0, 1);
        for (size_t i = 0; i < NUM_OPERATIONS; ++i) {
            if (dist_bool(rand) == 0) {
                // insert
                s.insert(ids[dist_ids(rand)]);
            } else {
                // lookup
                if (s.find(ids[dist_ids(rand)]) != s.end()) {
                    ++num_found;
                }
            }
        }
        std::cout << "num_found: " << num_found << std::endl;
    }

    auto end_marker = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_s = end_marker - start_marker;
    std::cout << "Benchmark completed in " << elapsed_s.count() << "s" << std::endl;
}

} // namespace detail

// Tests for coveo::lazy::set class
void set_tests()
{
    using namespace coveo_tests::lazy::detail;

    typedef coveo::lazy::set<int> int_set;
    std::vector<int> stdvector({ 42, 23 });
    std::initializer_list<int> initlist = { 66, 11 };
    std::vector<int> orderedstdvector({ 23, 42 });
    std::initializer_list<int> orderedinitlist = { 11, 66 };

    // Constructors
    int_set empty;
    int_set fromstdvector(stdvector.cbegin(), stdvector.cend());
    int_set fromstdvector_copy(fromstdvector);
    int_set willbeempty(fromstdvector);
    int_set fromstdvector_move(std::move(willbeempty));
    int_set frominitlist(initlist);

    // Assignment operators
    int_set fromstdvector_assign;
    fromstdvector_assign = fromstdvector;
    int_set willbeempty2(fromstdvector);
    int_set fromstdvector_moveassign;
    fromstdvector_moveassign = std::move(willbeempty2);
    int_set frominitlist_assign;
    frominitlist_assign = initlist;

    // Equality
    COVEO_ASSERT(containers_are_equal(fromstdvector, orderedstdvector));
    COVEO_ASSERT(containers_are_equal(fromstdvector, fromstdvector_copy));
    COVEO_ASSERT(containers_are_equal(fromstdvector, fromstdvector_move));
    COVEO_ASSERT(containers_are_equal(fromstdvector, fromstdvector_assign));
    COVEO_ASSERT(containers_are_equal(fromstdvector, fromstdvector_moveassign));
    COVEO_ASSERT(!containers_are_equal(fromstdvector, empty));
    COVEO_ASSERT(!containers_are_equal(fromstdvector, frominitlist));
    COVEO_ASSERT(!containers_are_equal(fromstdvector, frominitlist_assign));
    COVEO_ASSERT(containers_are_equal(frominitlist, orderedinitlist));
    COVEO_ASSERT(containers_are_equal(frominitlist, frominitlist_assign));
    COVEO_ASSERT(fromstdvector == fromstdvector_copy);
    COVEO_ASSERT(fromstdvector == fromstdvector_move);
    COVEO_ASSERT(fromstdvector == fromstdvector_assign);
    COVEO_ASSERT(fromstdvector == fromstdvector_moveassign);
    COVEO_ASSERT(fromstdvector != empty);
    COVEO_ASSERT(fromstdvector != frominitlist);
    COVEO_ASSERT(fromstdvector != frominitlist_assign);
    COVEO_ASSERT(frominitlist == frominitlist_assign);

    // Comparison
    COVEO_ASSERT(container_is_less_than(frominitlist, fromstdvector));
    COVEO_ASSERT(frominitlist <  fromstdvector);
    COVEO_ASSERT(frominitlist <= fromstdvector);
    COVEO_ASSERT(!(frominitlist >  fromstdvector));
    COVEO_ASSERT(!(frominitlist >= fromstdvector));

    // Iterators
    {
        auto it = fromstdvector.begin();
        COVEO_ASSERT(*it++ == 23);
        COVEO_ASSERT(*it++ == 42);
        COVEO_ASSERT( it   == fromstdvector.end());
    }
    {
        auto cit = fromstdvector.cbegin();
        COVEO_ASSERT(*cit++ == 23);
        COVEO_ASSERT(*cit++ == 42);
        COVEO_ASSERT( cit   == fromstdvector.cend());
    }
    {
        auto cit = static_cast<const int_set&>(fromstdvector).begin();
        COVEO_ASSERT(*cit++ == 23);
        COVEO_ASSERT(*cit++ == 42);
        COVEO_ASSERT( cit   == fromstdvector.cend());
    }
    {
        auto rit = fromstdvector.rbegin();
        COVEO_ASSERT(*rit++ == 42);
        COVEO_ASSERT(*rit++ == 23);
        COVEO_ASSERT( rit   == fromstdvector.rend());
    }
    {
        auto crit = fromstdvector.crbegin();
        COVEO_ASSERT(*crit++ == 42);
        COVEO_ASSERT(*crit++ == 23);
        COVEO_ASSERT( crit   == fromstdvector.crend());
    }
    {
        auto crit = static_cast<const int_set&>(fromstdvector).rbegin();
        COVEO_ASSERT(*crit++ == 42);
        COVEO_ASSERT(*crit++ == 23);
        COVEO_ASSERT( crit   == fromstdvector.crend());
    }

    // Size/capacity
    COVEO_ASSERT(empty.empty());
    COVEO_ASSERT(empty.size() == 0);
    COVEO_ASSERT(empty.max_size() != 0);
    COVEO_ASSERT(!fromstdvector.empty());
    COVEO_ASSERT(fromstdvector.size() == 2);

    // Capacity (available since we're using vector internally)
    {
        int_set local;
        local.reserve(32);
        COVEO_ASSERT(local.capacity() >= 32);
        local.shrink_to_fit();
        COVEO_ASSERT(local.capacity() >= 0);
    }

    // Lookups
    {
        COVEO_ASSERT(fromstdvector.count(23) == 1);
        COVEO_ASSERT(fromstdvector.count(24) == 0);
        
        auto it = fromstdvector.find(23);
        COVEO_ASSERT(it != fromstdvector.end());
        COVEO_ASSERT(*it == 23);
        it = fromstdvector.find(24);
        COVEO_ASSERT(it == fromstdvector.end());

        it = fromstdvector.lower_bound(23);
        COVEO_ASSERT(it != fromstdvector.end());
        COVEO_ASSERT(*it == 23);
        it = fromstdvector.lower_bound(24);
        COVEO_ASSERT(it != fromstdvector.end());
        COVEO_ASSERT(*it == 42);
        it = fromstdvector.lower_bound(42);
        COVEO_ASSERT(it != fromstdvector.end());
        COVEO_ASSERT(*it == 42);
        it = fromstdvector.lower_bound(99);
        COVEO_ASSERT(it == fromstdvector.end());

        it = fromstdvector.upper_bound(23);
        COVEO_ASSERT(it != fromstdvector.end());
        COVEO_ASSERT(*it == 42);
        it = fromstdvector.upper_bound(24);
        COVEO_ASSERT(it != fromstdvector.end());
        COVEO_ASSERT(*it == 42);
        it = fromstdvector.upper_bound(42);
        COVEO_ASSERT(it == fromstdvector.end());
        it = fromstdvector.upper_bound(99);
        COVEO_ASSERT(it == fromstdvector.end());

        auto it_pair = fromstdvector.equal_range(23);
        COVEO_ASSERT(it_pair.first != fromstdvector.end());
        COVEO_ASSERT(*it_pair.first == 23);
        COVEO_ASSERT(it_pair.second != fromstdvector.end());
        COVEO_ASSERT(*it_pair.second == 42);
        it_pair = fromstdvector.equal_range(24);
        COVEO_ASSERT(it_pair.first != fromstdvector.end());
        COVEO_ASSERT(*it_pair.first == 42);
        COVEO_ASSERT(it_pair.second != fromstdvector.end());
        COVEO_ASSERT(*it_pair.second == 42);
        it_pair = fromstdvector.equal_range(42);
        COVEO_ASSERT(it_pair.first != fromstdvector.end());
        COVEO_ASSERT(*it_pair.first == 42);
        COVEO_ASSERT(it_pair.second == fromstdvector.end());
        it_pair = fromstdvector.equal_range(99);
        COVEO_ASSERT(it_pair.first == fromstdvector.end());
        COVEO_ASSERT(it_pair.second == fromstdvector.end());
    }

    // Lookups with transparent comparator
    {
        typedef coveo::lazy::set<int, std::less<>> tr_int_set;
        tr_int_set localfromstdvector(stdvector.cbegin(), stdvector.cend());

        COVEO_ASSERT(localfromstdvector.count(Int(23)) == 1);
        COVEO_ASSERT(localfromstdvector.count(Int(24)) == 0);
        
        auto it = localfromstdvector.find(Int(23));
        COVEO_ASSERT(it != localfromstdvector.end());
        COVEO_ASSERT(*it == 23);
        it = localfromstdvector.find(Int(24));
        COVEO_ASSERT(it == localfromstdvector.end());

        it = localfromstdvector.lower_bound(Int(23));
        COVEO_ASSERT(it != localfromstdvector.end());
        COVEO_ASSERT(*it == 23);
        it = localfromstdvector.lower_bound(Int(24));
        COVEO_ASSERT(it != localfromstdvector.end());
        COVEO_ASSERT(*it == 42);
        it = localfromstdvector.lower_bound(Int(42));
        COVEO_ASSERT(it != localfromstdvector.end());
        COVEO_ASSERT(*it == 42);
        it = localfromstdvector.lower_bound(Int(99));
        COVEO_ASSERT(it == localfromstdvector.end());

        it = localfromstdvector.upper_bound(Int(23));
        COVEO_ASSERT(it != localfromstdvector.end());
        COVEO_ASSERT(*it == 42);
        it = localfromstdvector.upper_bound(Int(24));
        COVEO_ASSERT(it != localfromstdvector.end());
        COVEO_ASSERT(*it == 42);
        it = localfromstdvector.upper_bound(Int(42));
        COVEO_ASSERT(it == localfromstdvector.end());
        it = localfromstdvector.upper_bound(Int(99));
        COVEO_ASSERT(it == localfromstdvector.end());

        auto it_pair = localfromstdvector.equal_range(Int(23));
        COVEO_ASSERT(it_pair.first != localfromstdvector.end());
        COVEO_ASSERT(*it_pair.first == 23);
        COVEO_ASSERT(it_pair.second != localfromstdvector.end());
        COVEO_ASSERT(*it_pair.second == 42);
        it_pair = localfromstdvector.equal_range(Int(24));
        COVEO_ASSERT(it_pair.first != localfromstdvector.end());
        COVEO_ASSERT(*it_pair.first == 42);
        COVEO_ASSERT(it_pair.second != localfromstdvector.end());
        COVEO_ASSERT(*it_pair.second == 42);
        it_pair = localfromstdvector.equal_range(Int(42));
        COVEO_ASSERT(it_pair.first != localfromstdvector.end());
        COVEO_ASSERT(*it_pair.first == 42);
        COVEO_ASSERT(it_pair.second == localfromstdvector.end());
        it_pair = localfromstdvector.equal_range(Int(99));
        COVEO_ASSERT(it_pair.first == localfromstdvector.end());
        COVEO_ASSERT(it_pair.second == localfromstdvector.end());
    }

    // Modifiers
    {
        int_set local;
        auto lend = local.cend();

        int fourty_two = 42;
        local.insert(fourty_two);
        local.insert(23);

        int sixty_six = 66;
        local.insert(lend, sixty_six);
        local.insert(lend, 11);
        
        auto it = local.cbegin();
        COVEO_ASSERT(*it++ == 11);
        COVEO_ASSERT(*it++ == 23);
        COVEO_ASSERT(*it++ == 42);
        COVEO_ASSERT(*it++ == 66);
        COVEO_ASSERT( it   == local.cend());
    }
    {
        int_set local;
        
        local.insert(stdvector.cbegin(), stdvector.cend());
        local.insert(initlist);

        auto it = local.cbegin();
        COVEO_ASSERT(*it++ == 11);
        COVEO_ASSERT(*it++ == 23);
        COVEO_ASSERT(*it++ == 42);
        COVEO_ASSERT(*it++ == 66);
        COVEO_ASSERT( it   == local.cend());
    }
    {
        int_set local;
        auto lend = local.cend();

        local.emplace(42);
        const int twenty_three = 23;
        local.emplace(twenty_three);
        local.emplace_hint(lend, 66);
        const int eleven = 11;
        local.emplace_hint(lend, eleven);

        auto it = local.cbegin();
        COVEO_ASSERT(*it++ == 11);
        COVEO_ASSERT(*it++ == 23);
        COVEO_ASSERT(*it++ == 42);
        COVEO_ASSERT(*it++ == 66);
        COVEO_ASSERT( it   == local.cend());
    }
    {
        int_set local(fromstdvector);

        auto it = local.find(23);
        COVEO_ASSERT(it != local.end());
        auto next_it = local.erase(it);
        COVEO_ASSERT( next_it != local.end());
        COVEO_ASSERT(*next_it == 42);
        COVEO_ASSERT(local.find(23) == local.end());
        COVEO_ASSERT(local.size() == 1);
    }
    {
        int_set local(fromstdvector);

        auto it = local.find(23);
        auto end = local.end();
        COVEO_ASSERT(it != local.end());
        auto next_it = local.erase(it, end);
        COVEO_ASSERT(next_it == local.end());
        COVEO_ASSERT(local.empty());
    }
    {
        int_set local(fromstdvector);

        auto erased = local.erase(23);
        COVEO_ASSERT(erased == 1);
        COVEO_ASSERT(local.find(23) == local.end());
        COVEO_ASSERT(local.size() == 1);
        erased = local.erase(99);
        COVEO_ASSERT(erased == 0);
        COVEO_ASSERT(local.size() == 1);
    }
    {
        int_set local(fromstdvector);

        local.clear();
        COVEO_ASSERT(local.empty());
    }
    {
        int_set local1(fromstdvector);
        int_set local2;

        local1.swap(local2);
        COVEO_ASSERT(local1.empty());
        COVEO_ASSERT(local2 == fromstdvector);
    }
    {
        int_set local1(fromstdvector);
        int_set local2;

        using std::swap;
        swap(local1, local2);
        COVEO_ASSERT(local1.empty());
        COVEO_ASSERT(local2 == fromstdvector);
    }

    // Predicates / allocator
    {
        auto key_comp = fromstdvector.key_comp();
        COVEO_ASSERT( key_comp(23, 42));
        COVEO_ASSERT(!key_comp(42, 23));

        auto value_comp = fromstdvector.value_comp();
        COVEO_ASSERT( value_comp(23, 42));
        COVEO_ASSERT(!value_comp(42, 23));

        auto alloc = fromstdvector.get_allocator();

        auto key_eq = fromstdvector.key_eq();
        COVEO_ASSERT( key_eq(23, 23));
        COVEO_ASSERT(!key_eq(23, 42));

        auto value_eq = fromstdvector.value_eq();
        COVEO_ASSERT( value_eq(23, 23));
        COVEO_ASSERT(!value_eq(23, 67));
    }

    // Sorting
    {
        int_set local;

        local.emplace(42);
        local.emplace(23);
        COVEO_ASSERT(!local.sorted());
        local.sort();
        COVEO_ASSERT(local.sorted());
    }
    {
        int_set local;

        local.emplace(23);
        local.emplace(42);
        COVEO_ASSERT(local.sorted());
    }
}

// Tests for coveo::lazy::multiset class
void multiset_tests()
{
    using namespace coveo_tests::lazy::detail;

    typedef coveo::lazy::multiset<int> int_multiset;
    std::vector<int> stdvector({ 23, 42, 23 });
    std::initializer_list<int> initlist = { 11, 66, 11 };
    std::vector<int> orderedstdvector({ 23, 23, 42 });
    std::initializer_list<int> orderedinitlist = { 11, 11, 66 };

    // Constructors
    int_multiset empty;
    int_multiset fromstdvector(stdvector.cbegin(), stdvector.cend());
    int_multiset fromstdvector_copy(fromstdvector);
    int_multiset willbeempty(fromstdvector);
    int_multiset fromstdvector_move(std::move(willbeempty));
    int_multiset frominitlist(initlist);

    // Assignment operators
    int_multiset fromstdvector_assign;
    fromstdvector_assign = fromstdvector;
    int_multiset willbeempty2(fromstdvector);
    int_multiset fromstdvector_moveassign;
    fromstdvector_moveassign = std::move(willbeempty2);
    int_multiset frominitlist_assign;
    frominitlist_assign = initlist;

    // Equality
    COVEO_ASSERT(containers_are_equal(fromstdvector, orderedstdvector));
    COVEO_ASSERT(containers_are_equal(fromstdvector, fromstdvector_copy));
    COVEO_ASSERT(containers_are_equal(fromstdvector, fromstdvector_move));
    COVEO_ASSERT(containers_are_equal(fromstdvector, fromstdvector_assign));
    COVEO_ASSERT(containers_are_equal(fromstdvector, fromstdvector_moveassign));
    COVEO_ASSERT(!containers_are_equal(fromstdvector, empty));
    COVEO_ASSERT(!containers_are_equal(fromstdvector, frominitlist));
    COVEO_ASSERT(!containers_are_equal(fromstdvector, frominitlist_assign));
    COVEO_ASSERT(containers_are_equal(frominitlist, orderedinitlist));
    COVEO_ASSERT(containers_are_equal(frominitlist, frominitlist_assign));
    COVEO_ASSERT(fromstdvector == fromstdvector_copy);
    COVEO_ASSERT(fromstdvector == fromstdvector_move);
    COVEO_ASSERT(fromstdvector == fromstdvector_assign);
    COVEO_ASSERT(fromstdvector == fromstdvector_moveassign);
    COVEO_ASSERT(fromstdvector != empty);
    COVEO_ASSERT(fromstdvector != frominitlist);
    COVEO_ASSERT(fromstdvector != frominitlist_assign);
    COVEO_ASSERT(frominitlist == frominitlist_assign);

    // Comparison
    COVEO_ASSERT(container_is_less_than(frominitlist, fromstdvector));
    COVEO_ASSERT(frominitlist <  fromstdvector);
    COVEO_ASSERT(frominitlist <= fromstdvector);
    COVEO_ASSERT(!(frominitlist >  fromstdvector));
    COVEO_ASSERT(!(frominitlist >= fromstdvector));

    // Iterators
    {
        auto it = fromstdvector.begin();
        COVEO_ASSERT(*it++ == 23);
        COVEO_ASSERT(*it++ == 23);
        COVEO_ASSERT(*it++ == 42);
        COVEO_ASSERT( it   == fromstdvector.end());
    }
    {
        auto cit = fromstdvector.cbegin();
        COVEO_ASSERT(*cit++ == 23);
        COVEO_ASSERT(*cit++ == 23);
        COVEO_ASSERT(*cit++ == 42);
        COVEO_ASSERT( cit   == fromstdvector.cend());
    }
    {
        auto cit = static_cast<const int_multiset&>(fromstdvector).begin();
        COVEO_ASSERT(*cit++ == 23);
        COVEO_ASSERT(*cit++ == 23);
        COVEO_ASSERT(*cit++ == 42);
        COVEO_ASSERT( cit   == fromstdvector.cend());
    }
    {
        auto rit = fromstdvector.rbegin();
        COVEO_ASSERT(*rit++ == 42);
        COVEO_ASSERT(*rit++ == 23);
        COVEO_ASSERT(*rit++ == 23);
        COVEO_ASSERT( rit   == fromstdvector.rend());
    }
    {
        auto crit = fromstdvector.crbegin();
        COVEO_ASSERT(*crit++ == 42);
        COVEO_ASSERT(*crit++ == 23);
        COVEO_ASSERT(*crit++ == 23);
        COVEO_ASSERT( crit   == fromstdvector.crend());
    }
    {
        auto crit = static_cast<const int_multiset&>(fromstdvector).rbegin();
        COVEO_ASSERT(*crit++ == 42);
        COVEO_ASSERT(*crit++ == 23);
        COVEO_ASSERT(*crit++ == 23);
        COVEO_ASSERT( crit   == fromstdvector.crend());
    }

    // Size/capacity
    COVEO_ASSERT(empty.empty());
    COVEO_ASSERT(empty.size() == 0);
    COVEO_ASSERT(empty.max_size() != 0);
    COVEO_ASSERT(!fromstdvector.empty());
    COVEO_ASSERT(fromstdvector.size() == 3);

    // Capacity (available since we're using vector internally)
    {
        int_multiset local;
        local.reserve(32);
        COVEO_ASSERT(local.capacity() >= 32);
        local.shrink_to_fit();
        COVEO_ASSERT(local.capacity() >= 0);
    }

    // Lookups
    {
        COVEO_ASSERT(fromstdvector.count(23) == 2);
        COVEO_ASSERT(fromstdvector.count(42) == 1);
        COVEO_ASSERT(fromstdvector.count(24) == 0);
        
        auto it = fromstdvector.find(23);
        COVEO_ASSERT(it != fromstdvector.end());
        COVEO_ASSERT(*it == 23);
        it = fromstdvector.find(24);
        COVEO_ASSERT(it == fromstdvector.end());

        it = fromstdvector.lower_bound(23);
        COVEO_ASSERT(it != fromstdvector.end());
        COVEO_ASSERT(  *it == 23);
        COVEO_ASSERT(*++it == 23);
        it = fromstdvector.lower_bound(24);
        COVEO_ASSERT(it != fromstdvector.end());
        COVEO_ASSERT(*it == 42);
        it = fromstdvector.lower_bound(42);
        COVEO_ASSERT(it != fromstdvector.end());
        COVEO_ASSERT(*it == 42);
        it = fromstdvector.lower_bound(99);
        COVEO_ASSERT(it == fromstdvector.end());

        it = fromstdvector.upper_bound(23);
        COVEO_ASSERT(it != fromstdvector.end());
        COVEO_ASSERT(*it == 42);
        it = fromstdvector.upper_bound(24);
        COVEO_ASSERT(it != fromstdvector.end());
        COVEO_ASSERT(*it == 42);
        it = fromstdvector.upper_bound(42);
        COVEO_ASSERT(it == fromstdvector.end());
        it = fromstdvector.upper_bound(99);
        COVEO_ASSERT(it == fromstdvector.end());

        auto it_pair = fromstdvector.equal_range(23);
        COVEO_ASSERT(it_pair.first != fromstdvector.end());
        COVEO_ASSERT(  *it_pair.first == 23);
        COVEO_ASSERT(*++it_pair.first == 23);
        COVEO_ASSERT(it_pair.second != fromstdvector.end());
        COVEO_ASSERT(*it_pair.second == 42);
        it_pair = fromstdvector.equal_range(24);
        COVEO_ASSERT(it_pair.first != fromstdvector.end());
        COVEO_ASSERT(*it_pair.first == 42);
        COVEO_ASSERT(it_pair.second != fromstdvector.end());
        COVEO_ASSERT(*it_pair.second == 42);
        it_pair = fromstdvector.equal_range(42);
        COVEO_ASSERT(it_pair.first != fromstdvector.end());
        COVEO_ASSERT(*it_pair.first == 42);
        COVEO_ASSERT(it_pair.second == fromstdvector.end());
        it_pair = fromstdvector.equal_range(99);
        COVEO_ASSERT(it_pair.first == fromstdvector.end());
        COVEO_ASSERT(it_pair.second == fromstdvector.end());
    }

    // Lookups with transparent comparator
    {
        typedef coveo::lazy::multiset<int, std::less<>> tr_int_multiset;
        tr_int_multiset localfromstdvector(stdvector.cbegin(), stdvector.cend());

        COVEO_ASSERT(localfromstdvector.count(Int(23)) == 2);
        COVEO_ASSERT(localfromstdvector.count(Int(42)) == 1);
        COVEO_ASSERT(localfromstdvector.count(Int(24)) == 0);
        
        auto it = localfromstdvector.find(Int(23));
        COVEO_ASSERT(it != localfromstdvector.end());
        COVEO_ASSERT(*it == 23);
        it = localfromstdvector.find(Int(24));
        COVEO_ASSERT(it == localfromstdvector.end());

        it = localfromstdvector.lower_bound(Int(23));
        COVEO_ASSERT(it != localfromstdvector.end());
        COVEO_ASSERT(  *it == 23);
        COVEO_ASSERT(*++it == 23);
        it = localfromstdvector.lower_bound(Int(24));
        COVEO_ASSERT(it != localfromstdvector.end());
        COVEO_ASSERT(*it == 42);
        it = localfromstdvector.lower_bound(Int(42));
        COVEO_ASSERT(it != localfromstdvector.end());
        COVEO_ASSERT(*it == 42);
        it = localfromstdvector.lower_bound(Int(99));
        COVEO_ASSERT(it == localfromstdvector.end());

        it = localfromstdvector.upper_bound(Int(23));
        COVEO_ASSERT(it != localfromstdvector.end());
        COVEO_ASSERT(*it == 42);
        it = localfromstdvector.upper_bound(Int(24));
        COVEO_ASSERT(it != localfromstdvector.end());
        COVEO_ASSERT(*it == 42);
        it = localfromstdvector.upper_bound(Int(42));
        COVEO_ASSERT(it == localfromstdvector.end());
        it = localfromstdvector.upper_bound(Int(99));
        COVEO_ASSERT(it == localfromstdvector.end());

        auto it_pair = localfromstdvector.equal_range(Int(23));
        COVEO_ASSERT(it_pair.first != localfromstdvector.end());
        COVEO_ASSERT(  *it_pair.first == 23);
        COVEO_ASSERT(*++it_pair.first == 23);
        COVEO_ASSERT(it_pair.second != localfromstdvector.end());
        COVEO_ASSERT(*it_pair.second == 42);
        it_pair = localfromstdvector.equal_range(Int(24));
        COVEO_ASSERT(it_pair.first != localfromstdvector.end());
        COVEO_ASSERT(*it_pair.first == 42);
        COVEO_ASSERT(it_pair.second != localfromstdvector.end());
        COVEO_ASSERT(*it_pair.second == 42);
        it_pair = localfromstdvector.equal_range(Int(42));
        COVEO_ASSERT(it_pair.first != localfromstdvector.end());
        COVEO_ASSERT(*it_pair.first == 42);
        COVEO_ASSERT(it_pair.second == localfromstdvector.end());
        it_pair = localfromstdvector.equal_range(Int(99));
        COVEO_ASSERT(it_pair.first == localfromstdvector.end());
        COVEO_ASSERT(it_pair.second == localfromstdvector.end());
    }

    // Modifiers
    {
        int_multiset local;
        auto lend = local.cend();

        int fourty_two = 42;
        local.insert(fourty_two);
        local.insert(23);
        local.insert(23);

        int sixty_six = 66;
        local.insert(lend, sixty_six);
        local.insert(lend, 11);
        local.insert(lend, 11);
        
        auto it = local.cbegin();
        COVEO_ASSERT(*it++ == 11);
        COVEO_ASSERT(*it++ == 11);
        COVEO_ASSERT(*it++ == 23);
        COVEO_ASSERT(*it++ == 23);
        COVEO_ASSERT(*it++ == 42);
        COVEO_ASSERT(*it++ == 66);
        COVEO_ASSERT( it   == local.cend());
    }
    {
        int_multiset local;
        
        local.insert(stdvector.cbegin(), stdvector.cend());
        local.insert(initlist);

        auto it = local.cbegin();
        COVEO_ASSERT(*it++ == 11);
        COVEO_ASSERT(*it++ == 11);
        COVEO_ASSERT(*it++ == 23);
        COVEO_ASSERT(*it++ == 23);
        COVEO_ASSERT(*it++ == 42);
        COVEO_ASSERT(*it++ == 66);
        COVEO_ASSERT( it   == local.cend());
    }
    {
        int_multiset local;
        auto lend = local.cend();

        local.emplace(42);
        const int twenty_three = 23;
        local.emplace(twenty_three);
        local.emplace(23);
        local.emplace_hint(lend, 66);
        const int eleven = 11;
        local.emplace_hint(lend, eleven);
        local.emplace_hint(lend, 11);

        auto it = local.cbegin();
        COVEO_ASSERT(*it++ == 11);
        COVEO_ASSERT(*it++ == 11);
        COVEO_ASSERT(*it++ == 23);
        COVEO_ASSERT(*it++ == 23);
        COVEO_ASSERT(*it++ == 42);
        COVEO_ASSERT(*it++ == 66);
        COVEO_ASSERT( it   == local.cend());
    }
    {
        int_multiset local(fromstdvector);

        auto it = local.find(23);
        COVEO_ASSERT(it != local.end());
        auto next_it = local.erase(it);
        COVEO_ASSERT( next_it != local.end());
        COVEO_ASSERT(*next_it == 23);
        COVEO_ASSERT(local.size() == 2);
    }
    {
        int_multiset local(fromstdvector);

        auto it = local.find(23);
        auto end = local.end();
        COVEO_ASSERT(it != local.end());
        auto next_it = local.erase(it, end);
        COVEO_ASSERT(next_it == local.end());
        COVEO_ASSERT(local.empty());
    }
    {
        int_multiset local(fromstdvector);

        auto erased = local.erase(23);
        COVEO_ASSERT(erased == 2);
        COVEO_ASSERT(local.find(23) == local.end());
        COVEO_ASSERT(local.size() == 1);
        erased = local.erase(99);
        COVEO_ASSERT(erased == 0);
        COVEO_ASSERT(local.size() == 1);
    }
    {
        int_multiset local(fromstdvector);

        local.clear();
        COVEO_ASSERT(local.empty());
    }
    {
        int_multiset local1(fromstdvector);
        int_multiset local2;

        local1.swap(local2);
        COVEO_ASSERT(local1.empty());
        COVEO_ASSERT(local2 == fromstdvector);
    }
    {
        int_multiset local1(fromstdvector);
        int_multiset local2;

        using std::swap;
        swap(local1, local2);
        COVEO_ASSERT(local1.empty());
        COVEO_ASSERT(local2 == fromstdvector);
    }

    // Predicates / allocator
    {
        auto key_comp = fromstdvector.key_comp();
        COVEO_ASSERT( key_comp(23, 42));
        COVEO_ASSERT(!key_comp(42, 23));

        auto value_comp = fromstdvector.value_comp();
        COVEO_ASSERT( value_comp(23, 42));
        COVEO_ASSERT(!value_comp(42, 23));

        auto alloc = fromstdvector.get_allocator();

        auto key_eq = fromstdvector.key_eq();
        COVEO_ASSERT( key_eq(23, 23));
        COVEO_ASSERT(!key_eq(23, 42));

        auto value_eq = fromstdvector.value_eq();
        COVEO_ASSERT( value_eq(23, 23));
        COVEO_ASSERT(!value_eq(23, 67));
    }

    // Sorting
    {
        int_multiset local;

        local.emplace(23);
        local.emplace(42);
        local.emplace(23);
        COVEO_ASSERT(!local.sorted());
        local.sort();
        COVEO_ASSERT(local.sorted());
    }
    {
        int_multiset local;

        local.emplace(23);
        local.emplace(23);
        local.emplace(42);
        COVEO_ASSERT(local.sorted());
    }
}

// Benchmarks for coveo::lazy::set class
// Compares it with std::set
void set_benchmarks()
{
    typedef std::set<size_t>            set_type;
    typedef coveo::lazy::set<size_t>    lazy_set_type;

    detail::benchmark_inserts_and_set_operations<set_type>("std::set");
    detail::benchmark_inserts_and_set_operations<lazy_set_type>("coveo::lazy::set");

    std::cout << std::endl;

    detail::benchmark_mixed_set_operations<set_type>("std::set");
    detail::benchmark_mixed_set_operations<lazy_set_type>("coveo::lazy::set");
}

} // lazy
} // coveo_tests

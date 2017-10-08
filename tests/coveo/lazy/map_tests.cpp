// Copyright (c) 2015-2016, Coveo Solutions Inc.
// Distributed under the Apache License, Version 2.0 (see LICENSE).

#include "coveo/lazy/map_tests.h"

#include <coveo/lazy/map.h>
#include <coveo/test_framework.h>

#include <iostream>
#include <initializer_list>
#include <iterator>
#include <list>
#include <string>
#include <vector>
#include <utility>

namespace coveo_tests {
namespace lazy {
namespace detail {

// Container used to store test items.
template<class T>
using testcontainer = std::vector<T>;

// Helpers to compare containers without using native operators.
template<class C1, class C2>
bool containers_are_equal(const C1& c1, const C2& c2) {
    return std::equal(std::begin(c1), std::end(c1), std::begin(c2), std::end(c2));
}
template<class C1, class C2>
bool container_is_less_than(const C1& c1, const C2& c2) {
    return std::lexicographical_compare(std::begin(c1), std::end(c1), std::begin(c2), std::end(c2));
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

} // namespace detail

// Tests for coveo::lazy::map class
void map_tests()
{
    using namespace coveo_tests::lazy::detail;

    typedef coveo::lazy::map<int, std::string> int_string_map;
    typedef int_string_map::value_type value_type;
    testcontainer<value_type> stdvector({
        std::make_pair(42, "Life"),
        std::make_pair(23, "Hangar"),
    });
    std::initializer_list<value_type> initlist = {
        std::make_pair(66, "Route"),
        std::make_pair(11, "Math"),
    };
    testcontainer<value_type> orderedstdvector({
        std::make_pair(23, "Hangar"),
        std::make_pair(42, "Life"),
    });
    std::initializer_list<value_type> orderedinitlist = {
        std::make_pair(11, "Math"),
        std::make_pair(66, "Route"),
    };

    // Constructors
    int_string_map empty;
    int_string_map fromstdvector(stdvector.cbegin(), stdvector.cend());
    int_string_map fromstdvector_copy(fromstdvector);
    int_string_map willbeempty(fromstdvector);
    int_string_map fromstdvector_move(std::move(willbeempty));
    int_string_map frominitlist(initlist);

    // Assignment operators
    int_string_map fromstdvector_assign;
    fromstdvector_assign = fromstdvector;
    int_string_map willbeempty2(fromstdvector);
    int_string_map fromstdvector_moveassign;
    fromstdvector_moveassign = std::move(willbeempty2);
    int_string_map frominitlist_assign;
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

    // Element access
    {
        std::string& life = fromstdvector.at(42);
        COVEO_ASSERT(life == "Life");

        try {
            fromstdvector.at(99);
            COVEO_ASSERT_FALSE();
        } catch (const coveo::lazy::out_of_range&) {
            // normal
        }

        int_string_map localcopy(fromstdvector);
        std::string& hangar = localcopy[23];
        COVEO_ASSERT(hangar == "Hangar");
        std::string& newone = localcopy[99];
        COVEO_ASSERT(newone.empty());
        COVEO_ASSERT(localcopy.size() == 3);
    }

    // Iterators
    {
        auto it = fromstdvector.begin();
        COVEO_ASSERT(*it++ == value_type(23, "Hangar"));
        COVEO_ASSERT(*it++ == value_type(42, "Life"));
        COVEO_ASSERT( it   == fromstdvector.end());
    }
    {
        auto cit = fromstdvector.cbegin();
        COVEO_ASSERT(*cit++ == value_type(23, "Hangar"));
        COVEO_ASSERT(*cit++ == value_type(42, "Life"));
        COVEO_ASSERT( cit   == fromstdvector.cend());
    }
    {
        auto cit = static_cast<const int_string_map&>(fromstdvector).begin();
        COVEO_ASSERT(*cit++ == value_type(23, "Hangar"));
        COVEO_ASSERT(*cit++ == value_type(42, "Life"));
        COVEO_ASSERT( cit   == fromstdvector.cend());
    }
    {
        auto rit = fromstdvector.rbegin();
        COVEO_ASSERT(*rit++ == value_type(42, "Life"));
        COVEO_ASSERT(*rit++ == value_type(23, "Hangar"));
        COVEO_ASSERT( rit   == fromstdvector.rend());
    }
    {
        auto crit = fromstdvector.crbegin();
        COVEO_ASSERT(*crit++ == value_type(42, "Life"));
        COVEO_ASSERT(*crit++ == value_type(23, "Hangar"));
        COVEO_ASSERT( crit   == fromstdvector.crend());
    }
    {
        auto crit = static_cast<const int_string_map&>(fromstdvector).rbegin();
        COVEO_ASSERT(*crit++ == value_type(42, "Life"));
        COVEO_ASSERT(*crit++ == value_type(23, "Hangar"));
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
        int_string_map local;
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
        COVEO_ASSERT(*it == value_type(23, "Hangar"));
        it = fromstdvector.find(24);
        COVEO_ASSERT(it == fromstdvector.end());

        it = fromstdvector.lower_bound(23);
        COVEO_ASSERT(it != fromstdvector.end());
        COVEO_ASSERT(*it == value_type(23, "Hangar"));
        it = fromstdvector.lower_bound(24);
        COVEO_ASSERT(it != fromstdvector.end());
        COVEO_ASSERT(*it == value_type(42, "Life"));
        it = fromstdvector.lower_bound(42);
        COVEO_ASSERT(it != fromstdvector.end());
        COVEO_ASSERT(*it == value_type(42, "Life"));
        it = fromstdvector.lower_bound(99);
        COVEO_ASSERT(it == fromstdvector.end());

        it = fromstdvector.upper_bound(23);
        COVEO_ASSERT(it != fromstdvector.end());
        COVEO_ASSERT(*it == value_type(42, "Life"));
        it = fromstdvector.upper_bound(24);
        COVEO_ASSERT(it != fromstdvector.end());
        COVEO_ASSERT(*it == value_type(42, "Life"));
        it = fromstdvector.upper_bound(42);
        COVEO_ASSERT(it == fromstdvector.end());
        it = fromstdvector.upper_bound(99);
        COVEO_ASSERT(it == fromstdvector.end());

        auto it_pair = fromstdvector.equal_range(23);
        COVEO_ASSERT(it_pair.first != fromstdvector.end());
        COVEO_ASSERT(*it_pair.first == value_type(23, "Hangar"));
        COVEO_ASSERT(it_pair.second != fromstdvector.end());
        COVEO_ASSERT(*it_pair.second == value_type(42, "Life"));
        it_pair = fromstdvector.equal_range(24);
        COVEO_ASSERT(it_pair.first != fromstdvector.end());
        COVEO_ASSERT(*it_pair.first == value_type(42, "Life"));
        COVEO_ASSERT(it_pair.second != fromstdvector.end());
        COVEO_ASSERT(*it_pair.second == value_type(42, "Life"));
        it_pair = fromstdvector.equal_range(42);
        COVEO_ASSERT(it_pair.first != fromstdvector.end());
        COVEO_ASSERT(*it_pair.first == value_type(42, "Life"));
        COVEO_ASSERT(it_pair.second == fromstdvector.end());
        it_pair = fromstdvector.equal_range(99);
        COVEO_ASSERT(it_pair.first == fromstdvector.end());
        COVEO_ASSERT(it_pair.second == fromstdvector.end());
    }

    // Lookups with transparent comparator
    {
        typedef coveo::lazy::map<int, std::string, std::less<>> tr_int_string_map;
        tr_int_string_map localfromstdvector(stdvector.cbegin(), stdvector.cend());

        COVEO_ASSERT(localfromstdvector.count(Int(23)) == 1);
        COVEO_ASSERT(localfromstdvector.count(Int(24)) == 0);
        
        auto it = localfromstdvector.find(Int(23));
        COVEO_ASSERT(it != localfromstdvector.end());
        COVEO_ASSERT(*it == value_type(23, "Hangar"));
        it = localfromstdvector.find(Int(24));
        COVEO_ASSERT(it == localfromstdvector.end());

        it = localfromstdvector.lower_bound(Int(23));
        COVEO_ASSERT(it != localfromstdvector.end());
        COVEO_ASSERT(*it == value_type(23, "Hangar"));
        it = localfromstdvector.lower_bound(Int(24));
        COVEO_ASSERT(it != localfromstdvector.end());
        COVEO_ASSERT(*it == value_type(42, "Life"));
        it = localfromstdvector.lower_bound(Int(42));
        COVEO_ASSERT(it != localfromstdvector.end());
        COVEO_ASSERT(*it == value_type(42, "Life"));
        it = localfromstdvector.lower_bound(Int(99));
        COVEO_ASSERT(it == localfromstdvector.end());

        it = localfromstdvector.upper_bound(Int(23));
        COVEO_ASSERT(it != localfromstdvector.end());
        COVEO_ASSERT(*it == value_type(42, "Life"));
        it = localfromstdvector.upper_bound(Int(24));
        COVEO_ASSERT(it != localfromstdvector.end());
        COVEO_ASSERT(*it == value_type(42, "Life"));
        it = localfromstdvector.upper_bound(Int(42));
        COVEO_ASSERT(it == localfromstdvector.end());
        it = localfromstdvector.upper_bound(Int(99));
        COVEO_ASSERT(it == localfromstdvector.end());

        auto it_pair = localfromstdvector.equal_range(Int(23));
        COVEO_ASSERT(it_pair.first != localfromstdvector.end());
        COVEO_ASSERT(*it_pair.first == value_type(23, "Hangar"));
        COVEO_ASSERT(it_pair.second != localfromstdvector.end());
        COVEO_ASSERT(*it_pair.second == value_type(42, "Life"));
        it_pair = localfromstdvector.equal_range(Int(24));
        COVEO_ASSERT(it_pair.first != localfromstdvector.end());
        COVEO_ASSERT(*it_pair.first == value_type(42, "Life"));
        COVEO_ASSERT(it_pair.second != localfromstdvector.end());
        COVEO_ASSERT(*it_pair.second == value_type(42, "Life"));
        it_pair = localfromstdvector.equal_range(Int(42));
        COVEO_ASSERT(it_pair.first != localfromstdvector.end());
        COVEO_ASSERT(*it_pair.first == value_type(42, "Life"));
        COVEO_ASSERT(it_pair.second == localfromstdvector.end());
        it_pair = localfromstdvector.equal_range(Int(99));
        COVEO_ASSERT(it_pair.first == localfromstdvector.end());
        COVEO_ASSERT(it_pair.second == localfromstdvector.end());
    }

    // Modifiers
    {
        int_string_map local;
        auto lend = local.cend();

        value_type fourty_two(42, "Life");
        local.insert(fourty_two);
        local.insert(std::make_pair(23, "Hangar"));

        value_type sixty_six(66, "Route");
        local.insert(lend, sixty_six);
        local.insert(lend, std::make_pair(11, "Math"));
        
        auto it = local.cbegin();
        COVEO_ASSERT(*it++ == value_type(11, "Math"));
        COVEO_ASSERT(*it++ == value_type(23, "Hangar"));
        COVEO_ASSERT(*it++ == value_type(42, "Life"));
        COVEO_ASSERT(*it++ == value_type(66, "Route"));
        COVEO_ASSERT( it   == local.cend());
    }
    {
        int_string_map local;
        
        local.insert(stdvector.cbegin(), stdvector.cend());
        local.insert(initlist);

        auto it = local.cbegin();
        COVEO_ASSERT(*it++ == value_type(11, "Math"));
        COVEO_ASSERT(*it++ == value_type(23, "Hangar"));
        COVEO_ASSERT(*it++ == value_type(42, "Life"));
        COVEO_ASSERT(*it++ == value_type(66, "Route"));
        COVEO_ASSERT( it   == local.cend());
    }
    {
        int_string_map local;
        auto lend = local.cend();

        local.emplace(42, "Life");
        const std::string hangar("Hangar");
        local.emplace(23, hangar);
        local.emplace_hint(lend, 66, "Route");
        const std::string math("Math");
        local.emplace_hint(lend, 11, math);

        auto it = local.cbegin();
        COVEO_ASSERT(*it++ == value_type(11, "Math"));
        COVEO_ASSERT(*it++ == value_type(23, "Hangar"));
        COVEO_ASSERT(*it++ == value_type(42, "Life"));
        COVEO_ASSERT(*it++ == value_type(66, "Route"));
        COVEO_ASSERT( it   == local.cend());
    }
    {
        int_string_map local(fromstdvector);

        auto it = local.find(23);
        COVEO_ASSERT(it != local.end());
        auto next_it = local.erase(it);
        COVEO_ASSERT( next_it != local.end());
        COVEO_ASSERT(*next_it == value_type(42, "Life"));
        COVEO_ASSERT(local.find(23) == local.end());
        COVEO_ASSERT(local.size() == 1);
    }
    {
        int_string_map local(fromstdvector);

        auto it = local.find(23);
        auto end = local.end();
        COVEO_ASSERT(it != local.end());
        auto next_it = local.erase(it, end);
        COVEO_ASSERT(next_it == local.end());
        COVEO_ASSERT(local.empty());
    }
    {
        int_string_map local(fromstdvector);

        auto erased = local.erase(23);
        COVEO_ASSERT(erased == 1);
        COVEO_ASSERT(local.find(23) == local.end());
        COVEO_ASSERT(local.size() == 1);
        erased = local.erase(99);
        COVEO_ASSERT(erased == 0);
        COVEO_ASSERT(local.size() == 1);
    }
    {
        int_string_map local(fromstdvector);

        local.clear();
        COVEO_ASSERT(local.empty());
    }
    {
        int_string_map local1(fromstdvector);
        int_string_map local2;

        local1.swap(local2);
        COVEO_ASSERT(local1.empty());
        COVEO_ASSERT(local2 == fromstdvector);
    }
    {
        int_string_map local1(fromstdvector);
        int_string_map local2;

        using std::swap;
        swap(local1, local2);
        COVEO_ASSERT(local1.empty());
        COVEO_ASSERT(local2 == fromstdvector);
    }

    // Modifiers available only for non-multi maps
    {
        int_string_map local;

        auto res = local.insert_or_assign(23, "Hangar");
        COVEO_ASSERT(res.second);
        COVEO_ASSERT(res.first != local.end());
        COVEO_ASSERT(*res.first == value_type(23, "Hangar"));

        const std::string shuck = "Shuck";
        res = local.insert_or_assign(23, shuck);
        COVEO_ASSERT(!res.second);
        COVEO_ASSERT(res.first != local.end());
        COVEO_ASSERT(*res.first == value_type(23, shuck));
    }
    {
        int_string_map local;

        auto res = local.try_emplace(23, "Hangar");
        COVEO_ASSERT(res.second);
        COVEO_ASSERT(res.first != local.end());
        COVEO_ASSERT(*res.first == value_type(23, "Hangar"));

        const std::string shuck = "Shuck";
        res = local.try_emplace(23, shuck);
        COVEO_ASSERT(!res.second);
        COVEO_ASSERT(res.first != local.end());
        COVEO_ASSERT(*res.first == value_type(23, "Hangar"));
    }

    // Predicates / allocator
    {
        auto key_comp = fromstdvector.key_comp();
        COVEO_ASSERT( key_comp(23, 42));
        COVEO_ASSERT(!key_comp(42, 23));

        auto value_comp = fromstdvector.value_comp();
        COVEO_ASSERT( value_comp(value_type(23, "Hangar"), value_type(42, "Life")));
        COVEO_ASSERT(!value_comp(value_type(42, "Life"), value_type(23, "Hangar")));

        auto alloc = fromstdvector.get_allocator();

        auto key_eq = fromstdvector.key_eq();
        COVEO_ASSERT( key_eq(23, 23));
        COVEO_ASSERT(!key_eq(23, 42));

        auto value_eq = fromstdvector.value_eq();
        COVEO_ASSERT( value_eq(value_type(23, "Hangar"), value_type(23, "Shuck")));
        COVEO_ASSERT(!value_eq(value_type(23, "Hangar"), value_type(67, "Hangar")));
    }

    // Sorting
    {
        int_string_map local;

        local.emplace(42, "Life");
        local.emplace(23, "Hangar");
        COVEO_ASSERT(!local.sorted());
        local.sort();
        COVEO_ASSERT(local.sorted());
    }
    {
        int_string_map local;

        local.emplace(23, "Hangar");
        local.emplace(42, "Life");
        COVEO_ASSERT(local.sorted());
    }
}

// Tests for coveo::lazy::multimap class
void multimap_tests()
{
    using namespace coveo_tests::lazy::detail;

    typedef coveo::lazy::multimap<int, std::string> int_string_multimap;
    typedef int_string_multimap::value_type value_type;
    testcontainer<value_type> stdvector({
        std::make_pair(23, "Shuck"),
        std::make_pair(42, "Life"),
        std::make_pair(23, "Hangar"),
    });
    std::initializer_list<value_type> initlist = {
        std::make_pair(11, "Sentient"),
        std::make_pair(66, "Route"),
        std::make_pair(11, "Math"),
    };
    testcontainer<value_type> orderedstdvector({
        std::make_pair(23, "Shuck"),
        std::make_pair(23, "Hangar"),
        std::make_pair(42, "Life"),
    });
    std::initializer_list<value_type> orderedinitlist = {
        std::make_pair(11, "Sentient"),
        std::make_pair(11, "Math"),
        std::make_pair(66, "Route"),
    };

    // Constructors
    int_string_multimap empty;
    int_string_multimap fromstdvector(stdvector.cbegin(), stdvector.cend());
    int_string_multimap fromstdvector_copy(fromstdvector);
    int_string_multimap willbeempty(fromstdvector);
    int_string_multimap fromstdvector_move(std::move(willbeempty));
    int_string_multimap frominitlist(initlist);

    // Assignment operators
    int_string_multimap fromstdvector_assign;
    fromstdvector_assign = fromstdvector;
    int_string_multimap willbeempty2(fromstdvector);
    int_string_multimap fromstdvector_moveassign;
    fromstdvector_moveassign = std::move(willbeempty2);
    int_string_multimap frominitlist_assign;
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
        COVEO_ASSERT(*it++ == value_type(23, "Shuck"));
        COVEO_ASSERT(*it++ == value_type(23, "Hangar"));
        COVEO_ASSERT(*it++ == value_type(42, "Life"));
        COVEO_ASSERT( it   == fromstdvector.end());
    }
    {
        auto cit = fromstdvector.cbegin();
        COVEO_ASSERT(*cit++ == value_type(23, "Shuck"));
        COVEO_ASSERT(*cit++ == value_type(23, "Hangar"));
        COVEO_ASSERT(*cit++ == value_type(42, "Life"));
        COVEO_ASSERT( cit   == fromstdvector.cend());
    }
    {
        auto cit = static_cast<const int_string_multimap&>(fromstdvector).begin();
        COVEO_ASSERT(*cit++ == value_type(23, "Shuck"));
        COVEO_ASSERT(*cit++ == value_type(23, "Hangar"));
        COVEO_ASSERT(*cit++ == value_type(42, "Life"));
        COVEO_ASSERT( cit   == fromstdvector.cend());
    }
    {
        auto rit = fromstdvector.rbegin();
        COVEO_ASSERT(*rit++ == value_type(42, "Life"));
        COVEO_ASSERT(*rit++ == value_type(23, "Hangar"));
        COVEO_ASSERT(*rit++ == value_type(23, "Shuck"));
        COVEO_ASSERT( rit   == fromstdvector.rend());
    }
    {
        auto crit = fromstdvector.crbegin();
        COVEO_ASSERT(*crit++ == value_type(42, "Life"));
        COVEO_ASSERT(*crit++ == value_type(23, "Hangar"));
        COVEO_ASSERT(*crit++ == value_type(23, "Shuck"));
        COVEO_ASSERT( crit   == fromstdvector.crend());
    }
    {
        auto crit = static_cast<const int_string_multimap&>(fromstdvector).rbegin();
        COVEO_ASSERT(*crit++ == value_type(42, "Life"));
        COVEO_ASSERT(*crit++ == value_type(23, "Hangar"));
        COVEO_ASSERT(*crit++ == value_type(23, "Shuck"));
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
        int_string_multimap local;
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
        COVEO_ASSERT(*it == value_type(23, "Shuck"));
        it = fromstdvector.find(24);
        COVEO_ASSERT(it == fromstdvector.end());

        it = fromstdvector.lower_bound(23);
        COVEO_ASSERT(it != fromstdvector.end());
        COVEO_ASSERT(  *it == value_type(23, "Shuck"));
        COVEO_ASSERT(*++it == value_type(23, "Hangar"));
        it = fromstdvector.lower_bound(24);
        COVEO_ASSERT(it != fromstdvector.end());
        COVEO_ASSERT(*it == value_type(42, "Life"));
        it = fromstdvector.lower_bound(42);
        COVEO_ASSERT(it != fromstdvector.end());
        COVEO_ASSERT(*it == value_type(42, "Life"));
        it = fromstdvector.lower_bound(99);
        COVEO_ASSERT(it == fromstdvector.end());

        it = fromstdvector.upper_bound(23);
        COVEO_ASSERT(it != fromstdvector.end());
        COVEO_ASSERT(*it == value_type(42, "Life"));
        it = fromstdvector.upper_bound(24);
        COVEO_ASSERT(it != fromstdvector.end());
        COVEO_ASSERT(*it == value_type(42, "Life"));
        it = fromstdvector.upper_bound(42);
        COVEO_ASSERT(it == fromstdvector.end());
        it = fromstdvector.upper_bound(99);
        COVEO_ASSERT(it == fromstdvector.end());

        auto it_pair = fromstdvector.equal_range(23);
        COVEO_ASSERT(it_pair.first != fromstdvector.end());
        COVEO_ASSERT(  *it_pair.first == value_type(23, "Shuck"));
        COVEO_ASSERT(*++it_pair.first == value_type(23, "Hangar"));
        COVEO_ASSERT(it_pair.second != fromstdvector.end());
        COVEO_ASSERT(*it_pair.second == value_type(42, "Life"));
        it_pair = fromstdvector.equal_range(24);
        COVEO_ASSERT(it_pair.first != fromstdvector.end());
        COVEO_ASSERT(*it_pair.first == value_type(42, "Life"));
        COVEO_ASSERT(it_pair.second != fromstdvector.end());
        COVEO_ASSERT(*it_pair.second == value_type(42, "Life"));
        it_pair = fromstdvector.equal_range(42);
        COVEO_ASSERT(it_pair.first != fromstdvector.end());
        COVEO_ASSERT(*it_pair.first == value_type(42, "Life"));
        COVEO_ASSERT(it_pair.second == fromstdvector.end());
        it_pair = fromstdvector.equal_range(99);
        COVEO_ASSERT(it_pair.first == fromstdvector.end());
        COVEO_ASSERT(it_pair.second == fromstdvector.end());
    }

    // Lookups with transparent comparator
    {
        typedef coveo::lazy::multimap<int, std::string, std::less<>> tr_int_string_multimap;
        tr_int_string_multimap localfromstdvector(stdvector.cbegin(), stdvector.cend());

        COVEO_ASSERT(localfromstdvector.count(Int(23)) == 2);
        COVEO_ASSERT(localfromstdvector.count(Int(42)) == 1);
        COVEO_ASSERT(localfromstdvector.count(Int(24)) == 0);
        
        auto it = localfromstdvector.find(Int(23));
        COVEO_ASSERT(it != localfromstdvector.end());
        COVEO_ASSERT(*it == value_type(23, "Shuck"));
        it = localfromstdvector.find(Int(24));
        COVEO_ASSERT(it == localfromstdvector.end());

        it = localfromstdvector.lower_bound(Int(23));
        COVEO_ASSERT(it != localfromstdvector.end());
        COVEO_ASSERT(  *it == value_type(23, "Shuck"));
        COVEO_ASSERT(*++it == value_type(23, "Hangar"));
        it = localfromstdvector.lower_bound(Int(24));
        COVEO_ASSERT(it != localfromstdvector.end());
        COVEO_ASSERT(*it == value_type(42, "Life"));
        it = localfromstdvector.lower_bound(Int(42));
        COVEO_ASSERT(it != localfromstdvector.end());
        COVEO_ASSERT(*it == value_type(42, "Life"));
        it = localfromstdvector.lower_bound(Int(99));
        COVEO_ASSERT(it == localfromstdvector.end());

        it = localfromstdvector.upper_bound(Int(23));
        COVEO_ASSERT(it != localfromstdvector.end());
        COVEO_ASSERT(*it == value_type(42, "Life"));
        it = localfromstdvector.upper_bound(Int(24));
        COVEO_ASSERT(it != localfromstdvector.end());
        COVEO_ASSERT(*it == value_type(42, "Life"));
        it = localfromstdvector.upper_bound(Int(42));
        COVEO_ASSERT(it == localfromstdvector.end());
        it = localfromstdvector.upper_bound(Int(99));
        COVEO_ASSERT(it == localfromstdvector.end());

        auto it_pair = localfromstdvector.equal_range(Int(23));
        COVEO_ASSERT(it_pair.first != localfromstdvector.end());
        COVEO_ASSERT(  *it_pair.first == value_type(23, "Shuck"));
        COVEO_ASSERT(*++it_pair.first == value_type(23, "Hangar"));
        COVEO_ASSERT(it_pair.second != localfromstdvector.end());
        COVEO_ASSERT(*it_pair.second == value_type(42, "Life"));
        it_pair = localfromstdvector.equal_range(Int(24));
        COVEO_ASSERT(it_pair.first != localfromstdvector.end());
        COVEO_ASSERT(*it_pair.first == value_type(42, "Life"));
        COVEO_ASSERT(it_pair.second != localfromstdvector.end());
        COVEO_ASSERT(*it_pair.second == value_type(42, "Life"));
        it_pair = localfromstdvector.equal_range(Int(42));
        COVEO_ASSERT(it_pair.first != localfromstdvector.end());
        COVEO_ASSERT(*it_pair.first == value_type(42, "Life"));
        COVEO_ASSERT(it_pair.second == localfromstdvector.end());
        it_pair = localfromstdvector.equal_range(Int(99));
        COVEO_ASSERT(it_pair.first == localfromstdvector.end());
        COVEO_ASSERT(it_pair.second == localfromstdvector.end());
    }

    // Modifiers
    {
        int_string_multimap local;
        auto lend = local.cend();

        value_type fourty_two(42, "Life");
        local.insert(fourty_two);
        local.insert(value_type(23, "Shuck"));
        local.insert(std::make_pair(23, "Hangar"));

        value_type sixty_six(66, "Route");
        local.insert(lend, sixty_six);
        local.insert(lend, value_type(11, "Sentient"));
        local.insert(lend, std::make_pair(11, "Math"));
        
        auto it = local.cbegin();
        COVEO_ASSERT(*it++ == value_type(11, "Sentient"));
        COVEO_ASSERT(*it++ == value_type(11, "Math"));
        COVEO_ASSERT(*it++ == value_type(23, "Shuck"));
        COVEO_ASSERT(*it++ == value_type(23, "Hangar"));
        COVEO_ASSERT(*it++ == value_type(42, "Life"));
        COVEO_ASSERT(*it++ == value_type(66, "Route"));
        COVEO_ASSERT( it   == local.cend());
    }
    {
        int_string_multimap local;
        
        local.insert(stdvector.cbegin(), stdvector.cend());
        local.insert(initlist);

        auto it = local.cbegin();
        COVEO_ASSERT(*it++ == value_type(11, "Sentient"));
        COVEO_ASSERT(*it++ == value_type(11, "Math"));
        COVEO_ASSERT(*it++ == value_type(23, "Shuck"));
        COVEO_ASSERT(*it++ == value_type(23, "Hangar"));
        COVEO_ASSERT(*it++ == value_type(42, "Life"));
        COVEO_ASSERT(*it++ == value_type(66, "Route"));
        COVEO_ASSERT( it   == local.cend());
    }
    {
        int_string_multimap local;
        auto lend = local.cend();

        local.emplace(42, "Life");
        const int twenty_three = 23;
        local.emplace(twenty_three, "Shuck");
        const std::string hangar("Hangar");
        local.emplace(23, hangar);
        local.emplace_hint(lend, 66, "Route");
        const int eleven = 11;
        local.emplace_hint(lend, eleven, "Sentient");
        const std::string math("Math");
        local.emplace_hint(lend, 11, math);

        auto it = local.cbegin();
        COVEO_ASSERT(*it++ == value_type(11, "Sentient"));
        COVEO_ASSERT(*it++ == value_type(11, "Math"));
        COVEO_ASSERT(*it++ == value_type(23, "Shuck"));
        COVEO_ASSERT(*it++ == value_type(23, "Hangar"));
        COVEO_ASSERT(*it++ == value_type(42, "Life"));
        COVEO_ASSERT(*it++ == value_type(66, "Route"));
        COVEO_ASSERT( it   == local.cend());
    }
    {
        int_string_multimap local(fromstdvector);

        auto it = local.find(23);
        COVEO_ASSERT(it != local.end());
        auto next_it = local.erase(it);
        COVEO_ASSERT( next_it != local.end());
        COVEO_ASSERT(*next_it == value_type(23, "Hangar"));
        COVEO_ASSERT(local.size() == 2);
    }
    {
        int_string_multimap local(fromstdvector);

        auto it = local.find(23);
        auto end = local.end();
        COVEO_ASSERT(it != local.end());
        auto next_it = local.erase(it, end);
        COVEO_ASSERT(next_it == local.end());
        COVEO_ASSERT(local.empty());
    }
    {
        int_string_multimap local(fromstdvector);

        auto erased = local.erase(23);
        COVEO_ASSERT(erased == 2);
        COVEO_ASSERT(local.find(23) == local.end());
        COVEO_ASSERT(local.size() == 1);
        erased = local.erase(99);
        COVEO_ASSERT(erased == 0);
        COVEO_ASSERT(local.size() == 1);
    }
    {
        int_string_multimap local(fromstdvector);

        local.clear();
        COVEO_ASSERT(local.empty());
    }
    {
        int_string_multimap local1(fromstdvector);
        int_string_multimap local2;

        local1.swap(local2);
        COVEO_ASSERT(local1.empty());
        COVEO_ASSERT(local2 == fromstdvector);
    }
    {
        int_string_multimap local1(fromstdvector);
        int_string_multimap local2;

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
        COVEO_ASSERT( value_comp(value_type(23, "Hangar"), value_type(42, "Life")));
        COVEO_ASSERT(!value_comp(value_type(42, "Life"), value_type(23, "Hangar")));

        auto alloc = fromstdvector.get_allocator();

        auto key_eq = fromstdvector.key_eq();
        COVEO_ASSERT( key_eq(23, 23));
        COVEO_ASSERT(!key_eq(23, 42));

        auto value_eq = fromstdvector.value_eq();
        COVEO_ASSERT( value_eq(value_type(23, "Hangar"), value_type(23, "Shuck")));
        COVEO_ASSERT(!value_eq(value_type(23, "Hangar"), value_type(67, "Hangar")));
    }

    // Sorting
    {
        int_string_multimap local;

        local.emplace(23, "Shuck");
        local.emplace(42, "Life");
        local.emplace(23, "Hangar");
        COVEO_ASSERT(!local.sorted());
        local.sort();
        COVEO_ASSERT(local.sorted());
    }
    {
        int_string_multimap local;

        local.emplace(23, "Shuck");
        local.emplace(23, "Hangar");
        local.emplace(42, "Life");
        COVEO_ASSERT(local.sorted());
    }
}

} // lazy
} // coveo_tests

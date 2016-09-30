# `coveo::lazy` [![Build Status](https://travis-ci.org/coveo/lazy.svg?branch=master)](https://travis-ci.org/coveo/lazy) [![GitHub license](https://img.shields.io/badge/license-Apache%202-blue.svg)](https://raw.githubusercontent.com/coveo/lazy/master/LICENSE)
A C++ library of associative containers, like `std::set`, that perform lazy sorting.

## TL;DR
```c++
#include <coveo/lazy/map.h>
#include <iostream>
#include <string>

int main()
{
  // Same API as std::map, except that...
  coveo::lazy::map<int, std::string> m;
  
  // ...insert/emplace return void
  m.emplace(42, "Life");
  m.emplace(23, "Hangar");
  m.emplace(66, "Route");
  
  for (auto&& elem : m) {
    std::cout << "(" elem.first << ", " << elem.second << ") ";
  }
  std::cout << std::endl;
  // Prints:
  // (23, Hangar) (42, Life) (66, Route)

  return 0;
}
```

## Installing
The library is header-only. Therefore, to add it to your project, simply copy the content of the [`lib`](https://github.com/coveo/lazy/tree/master/lib) directory to a suitable place in your structure and add that path to your include paths. Look at the [`test`](https://github.com/coveo/lazy/tree/master/build) projects/makefiles for examples.

## Compiler support
`coveo::lazy` requires a C++ compiler that is fairly up-to-date with the C++11/14 standard. It has been successfully tested with the following compilers; YMMV.

* Microsoft Visual Studio 2015 Update 3 (on Windows 7)
* GCC 5.3.1 (on Ubuntu 16.04)
* Clang 3.8.0 (on Ubuntu 16.04)

If you have information about other compilers/versions, feel free to send it our way and we'll update the above list.

## Differences with the STL
`coveo::lazy` provides classes that mimick `std::set/multiset/map/multimap`, with a couple of differences.

* In order to support lazy sorting, `insert` and `emplace` add elements at the end without checking for exisiting elements; because of this, their return value is `void`.
  * Because of this, `std::insert_iterator` won't work with the lazy containers. Instead, use `coveo::lazy::insert_iterator` (via `coveo::lazy::inserter`).
* `coveo::lazy::map/multimap` use a custom allocator (`coveo::lazy::map_allocator`) instead of `std::allocator`. If you need to override the allocator, you need to use it also (see the `lazy_sorted_container.h` header for details).

Everything else should work as-is; if you notice something behaving differently, please report it and we'll investigate.

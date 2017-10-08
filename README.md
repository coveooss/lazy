# `coveo::lazy` [![Build Status](https://travis-ci.org/coveo/lazy.svg?branch=master)](https://travis-ci.org/coveo/lazy) [![Build status](https://ci.appveyor.com/api/projects/status/msiu2x343n348hv7/branch/master?svg=true)](https://ci.appveyor.com/project/clechasseur/lazy-dpgmg/branch/master) [![Build status](https://ci.appveyor.com/api/projects/status/fw689b9ppio7i3il/branch/master?svg=true)](https://ci.appveyor.com/project/clechasseur/lazy/branch/master) [![GitHub license](https://img.shields.io/badge/license-Apache%202-blue.svg)](https://raw.githubusercontent.com/coveo/lazy/master/LICENSE)
A C++14 library of associative containers, like `std::set`, that perform lazy sorting.

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
The library is header-only. Therefore, to add it to your project, simply copy the content of the [`lib`](https://github.com/coveo/lazy/tree/master/lib) directory to a suitable place in your structure and add that path to your include paths. Look at the `test` project/makefile for examples.

## Compiler support
`coveo::lazy` requires a C++ compiler that is fairly up-to-date with the C++11/14 standard. It has been successfully tested with the following compilers; YMMV.

* Microsoft Visual Studio 2015 Update 3
* GCC 5.3.1
* Clang 3.8.0

## MOAR?

Read further documentation and get API reference here: https://coveo.github.io/lazy

## License

Licensed under the [Apache License, Version 2.0](https://github.com/coveo/lazy/blob/master/LICENSE).

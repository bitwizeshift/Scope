
[![Release](https://img.shields.io/github/release/bitwizeshift/Scope.svg)](https://github.com/bitwizeshift/Scope/releases/latest)
[![License](https://img.shields.io/badge/License-BSL--1.0-blue.svg)](https://raw.githubusercontent.com/bitwizeshift/Scope/master/LICENSE)
[![Github Issues](https://img.shields.io/github/issues/bitwizeshift/Scope.svg)](https://github.com/bitwizeshift/Scope/issues)

# {Scope}

**{Scope}** is a modern utility library for managing resources using RAII.
This library features a permissive license ([Boost Software License](#license)),
and is only a single header -- making it simple to drop into any project.
This is compatible with any modern version of C++ provided it supports
C++11 or greater.

**Note:** This library is an implementation of the
[p0052r6 proposal paper][1] that introduces the `<scope>` header and utilities
to the C++ standard, but is written in C++11 with support for C++17
type-deduced constructors -- rather than only supporting C++17 features.

## Table of Contents

* [Features](#features) \
  A summary of all existing features in **{Scope}**
* [API Reference](https://bitwizeshift.github.io/Scope/api/latest/) \
  For doxygen-generated API information
* [Tutorials](doc/tutorial.md) \
  For tutorials on how to use **{Scope}**
* [How to install](doc/installing.md) \
  For a quick guide on how to install/use this in other projects
* [Legal](doc/legal.md) \
  Information about how to attribute this project
* [Contributing Guidelines](.github/CONTRIBUTING.md) \
  Guidelines for how to contribute to this project

## Features

- [x] Easily manage any C-style resources in an idiomatic way with
      `unique_resource`
- [x] Handle writing clean-up code exactly once with the various scope guards;
      keep that code `DRY`!
- [x] Works with C++11, and is fully functional in C++17
- [x] Written with portable C++ code, and tested on various compilers and
      operating systems
- [x] Only a single-header -- drop it in anywhere!
- [x] Super permissive license with the [Boost License](#license)

[1]: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0052r6.pdf

### Scope Guards

Scope guards allow for deferring an action until the end of a given scope.
This can be useful for various things:

* Logging
* Cleaning up resources in a function with multiple exit cases
* Creating transactional support by 'rolling-back' changes on failure
* etc

Three different scope guards exist for handling different exit strategies:

* **`scope_exit`**: Executes the callback every time
* **`scope_success`**: Executes the callback only on successful scope exit
  (no exceptions thrown)
* **`scope_fail`**: Executes the callback only on exceptional cases, when
  a scope is terminating due to a thrown exception.

### `unique_resource`

The `unique_resource` type is a more general version of the C++ standard's
`unique_ptr`, allowing for any underlying resource type to be managed with
unique ownership. This utility is extremely helpful for managing resources
distributed by old C-style interfaces.

For example, this could be done to manage a POSIX file:

```c++
{
  auto file = ::scope::make_unique_resource(
    ::open(...),
    &::close
  );
  // decltyp(file) is ::scope::unique_resource<int,void(*)(int)<

  ...

} // calls ::close on scope
```

In general, this can be used for any type of resource that has a dedicated
cleanup function. The deleters can be functors, lambdas, or any function
pointer.

## Tested Compilers

The following compilers are currently being tested through
[continuous integration](#continuous-integration) with
[Travis](https://travis-ci.com/bitwizeshift/Scope) and
[AppVeyor](https://ci.appveyor.com/project/bitwizeshift/scope)

| Compiler              | Operating System                   |
|-----------------------|------------------------------------|
| `g++4.9`              | Ubuntu 14.04.5 LTS                 |
| `g++5`                | Ubuntu 14.04.5 LTS                 |
| `g++6`                | Ubuntu 14.04.5 LTS                 |
| `g++7`                | Ubuntu 14.04.5 LTS                 |
| `g++8`                | Ubuntu 14.04.5 LTS                 |
| `clang++-3.5`         | Ubuntu 14.04.5 LTS                 |
| `clang++-3.6`         | Ubuntu 14.04.5 LTS                 |
| `clang++-8.0`         | Ubuntu 14.04.5 LTS                 |
| Xcode-9 clang++       | Mac OSX 10.12.6 (Darwin 16.7.0)    |
| Xcode 9.1 clang++     | Mac OSX 10.12.6 (Darwin 16.7.0)    |
| Xcode 9.2 clang++     | Mac OSX 10.12.6 (Darwin 16.7.0)    |
| Xcode 9.3 clang++     | Mac OSX 10.13.3 (Darwin 17.4.0)    |
| Xcode 9.44 clang++    | Mac OSX 10.13.3 (Darwin 17.4.0)    |
| Xcode 10 clang++      | Mac OSX 10.13.6 (Darwin 17.7.0)    |
| Visual Studio 2015    | Windows Server 2012 R2             |
| Visual Studio 2017    | Windows Server 2016                |

## Continuous Integration

| **Build**     | **Status**      |
|---------------|-----------------|
| Ubuntu, macOS | [![Build Status](https://travis-ci.com/bitwizeshift/Scope.svg?branch=master)](https://travis-ci.com/bitwizeshift/Scope) |
| MSVC          | [![Build status](https://ci.appveyor.com/api/projects/status/wj0frj0ld1xh0ltk/branch/master?svg=true)](https://ci.appveyor.com/project/bitwizeshift/scope/branch/master) |

## Current Releases

| **Host**            | **Release**      |
|---------------------|------------------|
| Github              | [![Release](https://img.shields.io/github/release/bitwizeshift/Scope.svg)](https://github.com/bitwizeshift/Scope/releases/latest) |
| Conan               | [ ![Release](https://api.bintray.com/packages/bitwizeshift/Scope/Scope%3Ascope/images/download.svg) ](https://bintray.com/bitwizeshift/Scope/Scope%3Ascope/_latestVersion) |

## License

<img align="right" src="http://opensource.org/trademarks/opensource/OSI-Approved-License-100x137.png">

**{Scope}** is licensed under the
[Boost Software License 1.0](https://www.boost.org/users/license.html):

> Boost Software License - Version 1.0 - August 17th, 2003
>
> Permission is hereby granted, free of charge, to any person or organization
> obtaining a copy of the software and accompanying documentation covered by
> this license (the "Software") to use, reproduce, display, distribute,
> execute, and transmit the Software, and to prepare derivative works of the
> Software, and to permit third-parties to whom the Software is furnished to
> do so, all subject to the following:
>
> The copyright notices in the Software and this entire statement, including
> the above license grant, this restriction and the following disclaimer,
> must be included in all copies of the Software, in whole or in part, and
> all derivative works of the Software, unless such copies or derivative
> works are solely in the form of machine-executable object code generated by
> a source language processor.
>
> THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
> IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
> FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
> SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
> FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
> ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
> DEALINGS IN THE SOFTWARE.
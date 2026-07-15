# clap

A lightweight, header-only C++ command line argument parser.

## Requirements

C++20 or later.

## Install

clap ships as a single header. You only need `include/clap.hpp`.

### Option 1: download the header

Download the header into your project:

```sh
curl -O https://raw.githubusercontent.com/lucaspujol/clap/main/include/clap.hpp
```

In exactly one `.cpp` file, define `CLAP_IMPLEMENTATION` before including it:

```cpp
#define CLAP_IMPLEMENTATION
#include "clap.hpp"
```

In every other file, include it without the define:

```cpp
#include "clap.hpp"
```

### Option 2: CMake FetchContent

```cmake
include(FetchContent)
FetchContent_Declare(clap
  GIT_REPOSITORY https://github.com/lucaspujol/clap.git
  GIT_TAG main)
FetchContent_MakeAvailable(clap)

target_link_libraries(your_app PRIVATE clap)
```

Linking `clap` puts the header on your include path. You still define
`CLAP_IMPLEMENTATION` in one `.cpp` file.

## Example

```cpp
#define CLAP_IMPLEMENTATION
#include "clap.hpp"

#include <iostream>

int main(int argc, char** argv) {
    clap::App app(argv[0], "example program");

    auto& name = app.option<std::string>("-n,--name", "your name")
                     .default_value("world");

    try {
        app.parse(argc, argv);
    } catch (const clap::HelpRequested&) {
        return 0;
    } catch (const clap::ClapException& e) {
        std::cerr << app.usage() << "\nError: " << e.what() << "\n";
        return 1;
    }

    std::cout << "Hello, " << name.get() << "\n";
    return 0;
}
```

## Building the examples and tests

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

Each example builds into its own folder under `examples/`.

## Features

- Flags. Boolean switches like `-v` or `--verbose`. Registered with
  `app.flag(...)` and read with a bool conversion.
- Typed options. Values parsed into a type, for example
  `app.option<int>("-c,--count", ...)`. Read with `.get()`.
- Default values. `option<int>(...).default_value(10)` uses the default when
  the option is absent.
- Positional arguments. Order-based arguments with no dash, registered with
  `app.positional<T>(...)`.
- Repeatable multi-value options. Pass the same flag more than once to build a
  list, for example `-t a -t b`. Registered with `app.multi_option<T>(...)`.
- Required and optional arguments. Mark an argument required with
  `.required()`. Parsing fails if it is missing.
- Short flag clustering. Combine short flags like `-vf`, attach values like
  `-c10`, and accept negatives like `-c-5`.
- Long options with equals. Both `--count 10` and `--count=10` work.
- Custom value types. Teach clap your own type by specializing
  `clap::TypeName` and `clap::ParseValue`. See `examples/custom_type`.
- Typed error handling. Parsing throws specific exceptions such as
  `MissingValue`, `InvalidValue`, and `UnknownArgument`, all deriving from
  `ClapException`.
- Automatic help. A `-h,--help` flag is added by default and prints generated
  usage and option listings. It can be renamed with `help_flag(...)` or turned
  off with `no_auto_help()`.

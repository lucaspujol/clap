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

    auto& help = app.flag("-h,--help", "Show this help message");
    auto& name = app.option<std::string>("-n,--name", "your name")
                     .default_value("world");

    bool ok = app.parse(argc, argv);
    if (help) { std::cout << app.help();  return 0; }
    if (!ok)  { std::cerr << app.error(); return 1; }

    std::cout << "Hello, " << name.get() << "\n";
    return 0;
}
```

`parse` never throws on bad input. It fills every value it can, records the
first error, and returns `true` on success or `false` otherwise. You own the
help flag: register it like any other, then print `app.help()` when it is set.
Because the whole argv is parsed before you react, checking `help` first lets it
win over a missing-required error. Nothing prints or exits behind your back — if
you want that convenience, wrap the `App` in your own class (see
`examples/encapsulated`).

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
  the option is absent. For a fallback computed at runtime (from another
  argument, the environment, ...) read it with `.get_or(fallback)` instead.
- Positional arguments. Order-based arguments with no dash, registered with
  `app.positional<T>(...)`.
- Repeatable multi-value options. Pass the same flag more than once to build a
  list, for example `-t a -t b`. Registered with `app.multi_option<T>(...)`.
- Required and optional arguments. Mark an argument required with
  `.required()`. Parsing fails if it is missing.
- Short flag clustering. Combine short flags like `-vf` and attach values like
  `-c10`. Negatives work in the attached form (`-c-5`, `--count=-5`); a spaced
  `-c -5` is rejected because `-5` looks like a flag.
- Long options with equals. Both `--count 10` and `--count=10` work.
- Custom value types. Teach clap your own type by specializing
  `clap::TypeName` and `clap::ParseValue`. See `examples/custom_type`.
- Errors without exceptions. `parse` never throws on bad input: it returns
  `false` and records the first error. Read the printable message plus usage
  line with `app.error()`, and the category with `app.error_kind()`
  (`UnknownArgument`, `MissingValue`, `InvalidValue`, ...). Only misconfiguring
  the parser (duplicate or malformed names) throws — a `ConfigError` at
  registration, because that is a bug in your program, not the user's input.
- Help is just a flag. clap registers nothing automatically. Register
  `-h,--help` (or any name you like) yourself, and print `app.help()` when it is
  set. This keeps the names free when you want them and never forces behaviour
  on you. See `examples/custom_help`.

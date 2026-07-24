# clap

[![ci](https://github.com/lucaspujol/clap/actions/workflows/ci.yml/badge.svg)](https://github.com/lucaspujol/clap/actions/workflows/ci.yml)
[![release](https://img.shields.io/github/v/release/lucaspujol/clap?sort=semver)](https://github.com/lucaspujol/clap/releases/latest)
[![docs](https://img.shields.io/badge/docs-doxygen-blue)](https://lucaspujol.github.io/clap/)
![C++20](https://img.shields.io/badge/C%2B%2B-20-blue)
![header-only](https://img.shields.io/badge/header--only-yes-brightgreen)
[![license](https://img.shields.io/github/license/lucaspujol/clap)](LICENSE)

A lightweight, header-only C++ command line argument parser.

## Requirements

C++20 or later.

## Install

clap ships as a single header. You only need `include/clap.hpp`.

### Option 1: download the header

Grab the header from the [latest release](https://github.com/lucaspujol/clap/releases/latest)
(carries a `#define CLAP_VERSION` stamp and a `clap.hpp.sha256` you can verify against):

```sh
curl -LO https://github.com/lucaspujol/clap/releases/latest/download/clap.hpp
```

Or track the dev version from `main`:

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

This `#define` approach is to mimic other single-header libs, like 
STB and others. 

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
`CLAP_IMPLEMENTATION` in one `.cpp` file. This isnt the most efficient
way to include clap on your project but hey, you do you x)

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
first error, and returns `true` on success or `false` otherwise.

You own the help flag: register it like any other, then print `app.help()` 
when it is set.

Because the whole argv is parsed before you react, checking `help` first lets it
win over a missing-required error. Nothing prints or exits behind your back: if
you want that convenience, wrap the `App` in your own class (see
`examples/encapsulated` or `examples/struct_args`).

## Building the examples and tests

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

Each example builds into its own folder under `examples/`.

## Features

### Flags and options

A flag is a boolean switch. An option carries a value, parsed into the type you
ask for.

```cpp
auto& verbose = app.flag("-v,--verbose", "Enable verbose output");
auto& count   = app.option<int>("-c,--count", "How many");

if (verbose) { ... }        // flags read as a bool
int n = count.get();        // options return the parsed value
```

> Note: you can do `app.option<bool>`, but c'mon, use a flag.

`.default_value(10)` fills in when the flag is absent. If the fallback depends
on something only known at runtime, drop the default and read it with
`.get_or(fallback)` (ex, random seed, or value dependant on another argument).

### Help is just a flag

clap registers nothing behind your back. Register `-h,--help` yourself, print
`app.help()` when it's set. The names stay yours (`examples/custom_help`).

### Positionals and variadics

Positionals match by order, no dash.

```cpp
auto& input = app.positional<std::string>("input", "Input file");
// ./prog file.txt              -> input.get() == "file.txt"
```

You can also define **variadic** arguments. It must be the last positional-type 
argument defined. It collects every remaining token into a list. Same shape as 
`touch a b c`.

```cpp
auto& files = app.variadic<std::string>("files", "Files to process");
// ./prog a.txt b.txt c.txt     -> files.get() = {"a.txt", "b.txt", "c.txt"}
```
Only one variadic, nothing after it, else clap throws a `ConfigError` at registration.

Options repeat too. Pass the flag again to grow the list (`examples/multi_option`):

```cpp
auto& tags = app.multi_option<std::string>("-t,--tag", "Tag (repeat -t)");
// -t red -t green   ->   tags.get() == {"red", "green"}
```

### Required, optional, custom types

Everything is optional until you chain `.required()` (`examples/required_optional`).
Positional arguments are required by default, unless marked with a `.default_value()`

For a type clap doesn't know, specialize `clap::TypeName` and `clap::ParseValue`
and it works everywhere a built-in does (`examples/custom_type`).

### Constraining values

A type says *how* a value parses, not *which* values are allowed. `"jsn"` is a
perfectly fine string, it just isn't a format you support. Two ways to say so:

```cpp
app.option<std::string>("-f,--format", "output format")
   .choices({"json", "xml", "yaml"});

app.option<int>("-j,--jobs", "parallel jobs")
   .range(1, 64);
```

`.choices()` pins the value to a set, `.range()` to an inclusive `[lo, hi]`.
Anything outside fails with the same `InvalidValue` you get from any other bad
input, worded the same way, no hand-rolled compare-and-print block on your side.

`.choices()` also feeds the help: the option reads `<json|xml|yaml>` instead of
`<string>`, so the allowed values document themselves.

They chain like the rest, and work on positionals and lists too. On a list
`.range()` checks every element:

```cpp
app.positional<std::string>("mode", "run mode").choices({"fast", "safe"});
app.variadic<int>("ports", "ports to bind").range(1, 65535);
```

### Syntax clap understands

Short flags cluster (`-vf`) and take attached values (`-c10`). Long options
accept `--count 10` or `--count=10` (`examples/short_clusters`).

**Flag discarding.** putting a `/` after the dashes discards the following
flag. This acts like "commenting out" a flag to test your app quickly. 

> Note: the validation is still applied, so a wrong type for a discarded
flag will still give out an error

```
./prog -/v            # flag stays false
./prog --/count=3     # 3 is parsed and range-checked, then discarded
./prog --/count=abc   # error: count expects and int, got string
```

This is super convenient and one of the features that I love the most.
IMO, this should be standard, at shell level even. 

**End of options with `--`.** The standard POSIX signal for "stop reading
flags." Every token after it is positional, even dash-leading ones. Same `--`
you use with `grep` and `rm` (`examples/separator`).

### Negative numbers read as flags

clap can't tell `-5` from a flag, so a spaced negative is rejected:

```
./prog -c -5      # rejected
./prog -5         # Unknown argument, even for a positional
```

Options take the value attached or with `=`:

```
./prog -c-5
./prog --count=-5
```

Positionals have neither form, so use `--`:

```
./prog -- -5
```

### Errors are values, not exceptions

`parse` never throws on bad input. It fills every value it can, keeps the
**first** error, and returns `false`.

```cpp
if (!app.parse(argc, argv)) {
    std::cerr << app.error();     // message plus usage line
    return 1;
}
```

`app.error_kind()` gives the category to branch on (`UnknownArgument`,
`MissingValue`, `InvalidValue`, ...).

Misconfiguring the parser is the exception: duplicate names and the like throw a
`ConfigError` at registration, since that's your bug, not the user's input.

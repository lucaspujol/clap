# clap

[![ci](https://github.com/lucaspujol/clap/actions/workflows/ci.yml/badge.svg)](https://github.com/lucaspujol/clap/actions/workflows/ci.yml)
[![release](https://img.shields.io/github/v/release/lucaspujol/clap?sort=semver)](https://github.com/lucaspujol/clap/releases/latest)
[![docs](https://img.shields.io/badge/docs-doxygen-blue)](https://lucaspujol.github.io/clap/)
![C++20](https://img.shields.io/badge/C%2B%2B-20-blue)
![header-only](https://img.shields.io/badge/header--only-yes-brightgreen)
[![license](https://img.shields.io/github/license/lucaspujol/clap)](LICENSE)

A lightweight, header-only C++ command line argument parser.

## Requirements

C++20 or later. CI builds and tests every commit on GCC and Clang (Linux),
AppleClang (macOS) and MSVC (Windows).

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

Include it. That's it, in every file that needs it:

```cpp
#include "clap.hpp"
```

No `#define`, no implementation TU: everything is `inline`. Older code that
still defines `CLAP_IMPLEMENTATION` keeps working, the macro is just ignored.

### Option 2: CMake FetchContent

```cmake
include(FetchContent)
FetchContent_Declare(clap
  GIT_REPOSITORY https://github.com/lucaspujol/clap.git
  GIT_TAG main)
FetchContent_MakeAvailable(clap)

target_link_libraries(your_app PRIVATE clap::clap)
```

Linking `clap::clap` puts the header on your include path. This isnt the most
efficient way to include clap on your project but hey, you do you x)

### Option 3: install it, then find_package

```sh
cmake -S . -B build
cmake --install build --prefix /usr/local
```

That installs `clap.hpp` plus a CMake package next to it:

```cmake
find_package(clap 0.5 REQUIRED)
target_link_libraries(your_app PRIVATE clap::clap)
```

Same `clap::clap` target as FetchContent gives you, so downstream code doesn't
care which path it came from. The installed version is the one stamped into the
header at release; a build from `main` says `dev` and installs as `0.0.0`, which
satisfies no version request — install from a release tag if you want one.

## Example

```cpp
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

A flag also remembers how many times it was passed, so the classic `-v -vv -vvv`
verbosity ladder is one call, no counting on your side:

```cpp
switch (verbose.count()) {    // 0 when absent, 3 for -vvv
    case 0:  log_level = Error;   break;
    case 1:  log_level = Warn;    break;
    default: log_level = Debug;   break;
}
```

Discarded occurrences (`-/v`, below) don't count.

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

### Values from the environment

An option can name an environment variable to fall back on
(`examples/env_var`):

```cpp
app.option<int>("-p,--port", "port to listen on")
   .from_env("PORT")
   .default_value(8080);
```

Precedence is **argv > env > `default_value()` > unset**.

The env value parses and validates like any other, `.choices()` and `.range()`
included. `PORT=nope` is an `InvalidValue` that names the variable, so you don't
go hunting through argv for a `-p` you never passed.

It resolves after argv and before the required check, so the environment can
satisfy a `.required()` option. Options only, not positionals or lists.

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

A typo'd argument close to a registered one gets a suggestion for free:

```
./prog --forec
Error: Unknown argument: --forec
        did you mean '--force'?
```

Edit distance with transpositions, so `forec` costs one edit like a dropped
letter would. One edit tolerated per three characters, and names under three
characters never match: nothing close enough, no suggestion.

Misconfiguring the parser is the exception: duplicate names and the like throw a
`ConfigError` at registration, since that's your bug, not the user's input.

### Subcommands are two Apps

clap has no subcommand feature and isn't getting one. An `App` skips `args[0]`
like `argv[0]`, so handing a second `App` `(argc - 1, argv + 1)` makes the
subcommand name the new `argv[0]`, and that app sees only its own arguments
(`examples/subcommands`):

```cpp
int cmd_commit(int argc, char** argv) {
    clap::App sub("prog commit", "record changes");
    auto& msg = sub.option<std::string>("-m,--message", "commit message").required();

    if (!sub.parse(argc, argv)) { std::cerr << sub.error(); return 1; }
    // ...
}

int main(int argc, char** argv) {
    if (argc > 1 && std::string(argv[1]) == "commit")
        return cmd_commit(argc - 1, argv + 1);      // "commit" becomes argv[0]
    // ...
}
```

Each subcommand gets its own help, usage line and errors. The top-level app
parses what comes *before* the subcommand name, and lists them in its
description — clap has nothing to print them for you.

That's the whole trick, and its whole extent: clap doesn't know these apps are
related. No shared arguments, no `prog help commit`, no suggestion on a
misspelled subcommand. Fine for a few; for a real subcommand tree, use a parser
built for it (CLI11, cxxopts).

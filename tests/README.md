# Tests

## Where does my test go?

One file per area, matching the `area:` labels used on issues. Pick the file
whose subject your change is about:

| File | Covers |
|---|---|
| `parsing_flags.cpp` | flags (`-v`), short clusters (`-vf`, `-vc 10`) |
| `parsing_options.cpp` | value-taking options in every spelling, `required()`, `default_value()` |
| `parsing_positionals.cpp` | positional slots, the variadic slot, positional ordering rules |
| `parsing_multi.cpp` | repeatable options (`-n a -n b`) |
| `types_values.cpp` | token → `T` conversion, supported types |
| `types_constraints.cpp` | `.choices()` and `.range()` |
| `types_env.cpp` | `.from_env()` fallback |
| `help_output.cpp` | the help flag, the usage line, help annotations |
| `errors.cpp` | parse-time failures: `ErrorKind` and messages |
| `registration.cpp` | config-time failures (thrown `ConfigError`) |
| `api.cpp` | `parse()` return, `error()`, `help()`, `get()` on unset |

Fixing a bug? Add the test to the file for the area it was filed under, next to
the closest existing case. If nothing fits, a new `<area>_<topic>.cpp` is fine —
CMake globs `tests/*.cpp`, so no build change is needed.

## Writing one

```cpp
#include "support/assertions.hpp"
#include "support/standard_app.hpp"

struct Flags : StandardApp {};      // once per file, at the top

TEST_F(Flags, Short) {
    Argv a{"prog", "-v"};
    expect_ok(app, a);
    EXPECT_TRUE(verbose);
}
```

- Always `TEST_F`, never `TEST` — GTest forbids mixing the two in one suite.
- `StandardApp` already has one argument of every kind: `app`, `help`,
  `verbose`, `force`, `count`, `names`, `input`. Use them directly.
- Need a different setup? Declare a local `clap::App app{"prog", "d"}` at the
  top of the test. It shadows the fixture's, and the rest is unchanged.
- Assert with `expect_ok` / `expect_error` / `expect_help` rather than calling
  `app.parse()` — they print diagnostics when they fail.

The suite name is the first macro argument, so `--gtest_filter=Positionals.*`
and `ctest` output both group by behaviour.

## Layout

```
support/
  argv.hpp           Argv builder (mutable char** as main() receives)
  assertions.hpp     expect_ok / expect_error / expect_help
  standard_app.hpp   the StandardApp fixture
  clap_header.hpp    every test TU includes clap.hpp through here
clap_impl.cpp        the single TU that defines CLAP_IMPLEMENTATION
coverage_probe.cpp   coverage builds only; see the file for why
```

Tests build against the shipped single header in `include/`, not the sources in
`src/`, so the amalgamation is what gets exercised.

## Coverage

```
tools/coverage.sh
```

Watch **branch** and **function** coverage, not line coverage. Most of clap is
inline or templated, and code that is never called is never emitted, so it does
not appear in the line total at all.

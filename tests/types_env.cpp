// .from_env(): an option falls back to an environment variable when argv did
// not supply it. The value still goes through the normal conversion, so a bad
// env value is a parse error like any other.

#include "support/assertions.hpp"
#include "support/env.hpp"
#include "support/standard_app.hpp"

#include <cstdlib>

struct Env : StandardApp {};

TEST_F(Env, EnvFallbackUsedWhenAbsent) {
    clap::App app{"prog", "d"};
    auto &count = app.option<int>("-c,--count", "count").from_env("TEST_COUNT");
    setenv("TEST_COUNT", "42", 1);
    Argv a{"prog"};
    expect_ok(app, a);
    EXPECT_EQ(count.get(), 42);
}

TEST_F(Env, EnvFallbackOverriddenWhenPresent) {
    clap::App app{"prog", "d"};
    auto &count = app.option<int>("-c,--count", "count").from_env("TEST_COUNT");
    setenv("TEST_COUNT", "42", 1);
    Argv a{"prog", "-c", "10"};
    expect_ok(app, a);
    EXPECT_EQ(count.get(), 10);
}

TEST_F(Env, EnvFallbackInvalidValueThrows) {
    clap::App app{"prog", "d"};
    app.option<int>("-c,--count", "count").from_env("TEST_COUNT");
    setenv("TEST_COUNT", "notanumber", 1);
    Argv a{"prog"};
    expect_error(app, a, clap::ErrorKind::InvalidValue);
}

TEST_F(Env, EnvKeyNotSet) {
    clap::App app{"prog", "d"};
    auto& v = app.option<int>("-c,--count", "count").from_env("NOT_SET");
    Argv a{"prog"};
    expect_ok(app, a);
    EXPECT_THROW(v.get(), clap::MissingValue);
}

TEST_F(Env, FirstEnvErrorIsTheReportedOne) {
    clap::App app{"prog", "d"};
    app.option<int>("-c,--count", "count").from_env("TEST_COUNT_A");
    app.option<int>("-d,--depth", "depth").from_env("TEST_COUNT_B");
    setenv("TEST_COUNT_A", "nope", 1);
    setenv("TEST_COUNT_B", "alsonope", 1);
    Argv a{"prog"};
    expect_error(app, a, clap::ErrorKind::InvalidValue);
    EXPECT_NE(app.error().find("'nope'"), std::string::npos);
}

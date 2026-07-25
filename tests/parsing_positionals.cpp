// Positional slots: fixed ones, and the variadic slot that greedily collects
// whatever is left. Ordering rules (optional before required, anything after a
// variadic) are registration errors and live here too, since they are about
// how positionals compose.

#include "support/assertions.hpp"
#include "support/standard_app.hpp"

struct Positionals : StandardApp {};
struct Variadic    : StandardApp {};

// =============================================================================
// Positionals  (standard app: `input`, optional via default_value)
// =============================================================================

TEST_F(Positionals, Captured) {
    Argv a{"prog", "file.txt"};
    expect_ok(app, a);
    EXPECT_EQ(input.get(), "file.txt");
}

TEST_F(Positionals, StringAcceptsSpaces) {
    Argv a{"prog", "a b c"};
    expect_ok(app, a);
    EXPECT_EQ(input.get(), "a b c");
}

TEST_F(Positionals, TakesValueReturnsTrue) {
    EXPECT_TRUE(input.takes_value());
}


// --- custom apps: default_value makes a positional optional -----------------

TEST_F(Positionals, DefaultUsedWhenAbsent) {
    clap::App app{"prog", "d"};
    auto& out = app.positional<std::string>("output", "out").default_value("output.txt");
    Argv a{"prog"};
    expect_ok(app, a);
    EXPECT_EQ(out.get(), "output.txt");
}

TEST_F(Positionals, DefaultOverriddenWhenPresent) {
    clap::App app{"prog", "d"};
    auto& out = app.positional<std::string>("output", "out").default_value("output.txt");
    Argv a{"prog", "custom.ppm"};
    expect_ok(app, a);
    EXPECT_EQ(out.get(), "custom.ppm");
}

// --- custom apps: a positional without a default is required ----------------

TEST_F(Positionals, RequiredMissingReported) {
    clap::App app{"prog", "d"};
    app.positional<std::string>("scene", "scene file");
    Argv a{"prog"};
    expect_error(app, a, clap::ErrorKind::MissingRequiredValue);
}

TEST_F(Positionals, RequiredPresentParses) {
    clap::App app{"prog", "d"};
    auto& scene = app.positional<std::string>("scene", "scene file");
    Argv a{"prog", "scene.txt"};
    expect_ok(app, a);
    EXPECT_EQ(scene.get(), "scene.txt");
}

// --- custom apps: throws on empty value -------------------------------------

TEST_F(Positionals, GetBeforeParseThrows) {
    clap::App app{"prog", "d"};
    auto& in = app.positional<std::string>("input", "in");
    EXPECT_THROW(in.get(), clap::MissingValue);
}

// --- Value list security check ----------------------------------------------

TEST_F(Variadic, ValueListThrowsOnEmptyRequired) {
    clap::App app{"prog", "d"};
    auto &v = app.multi_option<int>("-n,--nums", "numbers")
                  .required();
    Argv a{"prog"};
    EXPECT_THROW(v.get(), clap::MissingValue);
}

// =============================================================================
// Variadic positional:  the last positional slot greedily collects the rest
// =============================================================================

TEST_F(Variadic, CollectsRemainingTokens) {
    clap::App app{"prog", "d"};
    auto& files = app.variadic<std::string>("files", "input files");
    Argv a{"prog", "a.txt", "b.txt", "c.txt"};
    expect_ok(app, a);
    ASSERT_EQ(files.get().size(), 3u);
    EXPECT_EQ(files.get()[0], "a.txt");
    EXPECT_EQ(files.get()[1], "b.txt");
    EXPECT_EQ(files.get()[2], "c.txt");
}

TEST_F(Variadic, FixedPositionalThenVariadic) {
    // First slot takes one token; the variadic eats everything after it.
    clap::App app{"prog", "d"};
    auto& fmt = app.positional<std::string>("format", "output format");
    auto& files = app.variadic<std::string>("files", "input files");
    Argv a{"prog", "json", "a", "b"};
    expect_ok(app, a);
    EXPECT_EQ(fmt.get(), "json");
    ASSERT_EQ(files.get().size(), 2u);
    EXPECT_EQ(files.get()[0], "a");
    EXPECT_EQ(files.get()[1], "b");
}

TEST_F(Variadic, EmptyAllowedWhenNotRequired) {
    clap::App app{"prog", "d"};
    auto& files = app.variadic<std::string>("files", "input files");
    Argv a{"prog"};
    expect_ok(app, a);
    EXPECT_TRUE(files.get().empty());      // optional + absent -> empty, no throw
}

TEST_F(Variadic, RequiredEmptyReported) {
    clap::App app{"prog", "d"};
    app.variadic<std::string>("files", "input files").required();
    Argv a{"prog"};
    expect_error(app, a, clap::ErrorKind::MissingRequiredValue);
}

TEST_F(Variadic, TypedConversion) {
    clap::App app{"prog", "d"};
    auto& nums = app.variadic<int>("nums", "numbers");
    Argv a{"prog", "1", "2", "3"};
    expect_ok(app, a);
    ASSERT_EQ(nums.get().size(), 3u);
    EXPECT_EQ(nums.get()[0], 1);
    EXPECT_EQ(nums.get()[2], 3);
}

TEST_F(Variadic, BadValueReported) {
    clap::App app{"prog", "d"};
    app.variadic<int>("nums", "numbers");
    Argv a{"prog", "1", "oops"};
    expect_error(app, a, clap::ErrorKind::InvalidValue);
}

TEST_F(Variadic, DashDashForcesLiteralCollection) {
    // Everything after "--" is positional, even flag-looking tokens.
    clap::App app{"prog", "d"};
    auto& files = app.variadic<std::string>("files", "input files");
    Argv a{"prog", "--", "-x", "--y"};
    expect_ok(app, a);
    ASSERT_EQ(files.get().size(), 2u);
    EXPECT_EQ(files.get()[0], "-x");
    EXPECT_EQ(files.get()[1], "--y");
}

TEST_F(Variadic, PositionalAfterVariadicRejected) {
    clap::App app{"prog", "d"};
    app.variadic<std::string>("files", "input files");
    EXPECT_THROW(app.positional<std::string>("trailing", "nope"), clap::ConfigError);
}

TEST_F(Variadic, SecondVariadicRejected) {
    clap::App app{"prog", "d"};
    app.variadic<std::string>("files", "input files");
    EXPECT_THROW(app.variadic<std::string>("more", "nope"), clap::ConfigError);
}

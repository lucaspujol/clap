// Positional slots: fixed ones, and the variadic slot that greedily collects
// whatever is left. Also how tokens are shared out when there are fewer of them
// than there are slots: required first, then the optionals left to right.
// A positional after a variadic is still a registration error, and lives here
// too, since it is about how positionals compose.

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

// --- custom apps: allocation when an optional sits before a required one ----
// Tokens are assigned after the walk, not by running index: the required ones
// are served first in declaration order, then the optionals split the surplus.

TEST_F(Positionals, OptionalBeforeRequiredKeepsDefaultWhenScarce) {
    clap::App app{"prog", "d"};
    auto& in  = app.positional<std::string>("in", "input").default_value("-");
    auto& out = app.positional<std::string>("out", "output");
    expect_ok(app, {"prog", "a"});
    EXPECT_EQ(in.get(), "-");     // skipped: no surplus to spend
    EXPECT_EQ(out.get(), "a");
}

TEST_F(Positionals, OptionalBeforeRequiredFillsWhenSurplus) {
    clap::App app{"prog", "d"};
    auto& in  = app.positional<std::string>("in", "input").default_value("-");
    auto& out = app.positional<std::string>("out", "output");
    expect_ok(app, {"prog", "a", "b"});
    EXPECT_EQ(in.get(), "a");
    EXPECT_EQ(out.get(), "b");
}

TEST_F(Positionals, OptionalBeforeRequiredStillReportsTheRequiredOne) {
    clap::App app{"prog", "d"};
    app.positional<std::string>("in", "input").default_value("-");
    app.positional<std::string>("out", "output");
    expect_error(app, {"prog"}, clap::ErrorKind::MissingRequiredValue);
}

// Declaration order breaks the ties: the leftmost optional eats first.
TEST_F(Positionals, SurplusGoesToTheLeftmostOptional) {
    clap::App app{"prog", "d"};
    auto& a = app.positional<std::string>("a", "a").default_value("da");
    auto& b = app.positional<std::string>("b", "b");
    auto& c = app.positional<std::string>("c", "c").default_value("dc");
    expect_ok(app, {"prog", "x", "y"});
    EXPECT_EQ(a.get(), "x");
    EXPECT_EQ(b.get(), "y");
    EXPECT_EQ(c.get(), "dc");
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

// --- custom apps: positionals are typed, not just strings -------------------

TEST_F(Positionals, TypedConversion) {
    clap::App app{"prog", "d"};
    auto& port = app.positional<int>("port", "port");
    Argv a{"prog", "8080"};
    expect_ok(app, a);
    EXPECT_EQ(port.get(), 8080);
}

TEST_F(Positionals, TypedBadValueReported) {
    clap::App app{"prog", "d"};
    app.positional<int>("port", "port");
    Argv a{"prog", "eighty"};
    expect_error(app, a, clap::ErrorKind::InvalidValue);
}

TEST_F(Positionals, TypedDefaultRenderedInHelp) {
    clap::App app{"prog", "d"};
    app.positional<int>("port", "port").default_value(8080);
    EXPECT_NE(app.help().find("(default: 8080)"), std::string::npos);
}

TEST_F(Positionals, TypedRangeRejectsOutOfBounds) {
    clap::App app{"prog", "d"};
    app.positional<int>("port", "port").range(1, 65535);
    Argv a{"prog", "70000"};
    expect_error(app, a, clap::ErrorKind::InvalidValue);
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

// An optional in front of a variadic fills first: it takes one token out of
// the surplus, the variadic takes whatever remains.
TEST_F(Variadic, OptionalBeforeVariadicFillsFirst) {
    clap::App app{"prog", "d"};
    auto& mode  = app.positional<std::string>("mode", "mode").default_value("fast");
    auto& files = app.variadic<std::string>("files", "input files");
    expect_ok(app, {"prog", "safe", "a", "b"});
    EXPECT_EQ(mode.get(), "safe");
    EXPECT_EQ(files.get(), (std::vector<std::string>{"a", "b"}));
}

// A required variadic reserves one token, so the optional in front of it yields
// rather than starving it.
TEST_F(Variadic, RequiredVariadicReservesATokenFromTheOptional) {
    clap::App app{"prog", "d"};
    auto& mode  = app.positional<std::string>("mode", "mode").default_value("fast");
    auto& files = app.variadic<std::string>("files", "input files").required();
    expect_ok(app, {"prog", "a"});
    EXPECT_EQ(mode.get(), "fast");
    EXPECT_EQ(files.get(), (std::vector<std::string>{"a"}));
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

TEST_F(Positionals, EmptyTokenIsAPositional) {
    clap::App app{"prog", "d"};
    auto& scene = app.positional<std::string>("scene", "scene file");
    Argv a{"prog", ""};
    expect_ok(app, a);
    EXPECT_EQ(scene.get(), "");
}

// The first "--" switches to positional mode; a later one is just a value.
TEST_F(Variadic, SecondDashDashIsCollectedLiterally) {
    clap::App app{"prog", "d"};
    auto& files = app.variadic<std::string>("files", "input files");
    Argv a{"prog", "--", "a", "--"};
    expect_ok(app, a);
    ASSERT_EQ(files.get().size(), 2u);
    EXPECT_EQ(files.get()[1], "--");
}

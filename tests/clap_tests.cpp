// clap — test suite
// =============================================================================
//
// HOW TO ADD A TEST
// -----------------
// 1. Find the section that matches the behaviour you are testing (long options,
//    short options, clusters, positionals, ...). Sections are ordered simple ->
//    advanced and each has a `// --- <name> ---` banner.
//
// 2. Write `TEST_F(<Section>, Name)`. Every section is a fixture subclass, so:
//
//    a) If `StandardApp` (below) already has the arguments you need, just use
//       its members: `app`, `help`, `count`, `names`, `input`, etc.
//
//    b) If you need a different configuration — a required argument, a custom
//       name, a registration error — declare a local `clap::App app{...}` at
//       the top of the test. It shadows the fixture's app; the rest is the same.
//
//    (GTest forbids mixing TEST and TEST_F in one suite, hence: always TEST_F.)
//
// 3. Assert with the helpers: expect_ok / expect_error / expect_help. They wrap
//    parse() and print useful diagnostics on failure.
//
// The GTest suite name (first macro argument) is the section, so `ctest` output
// and `--gtest_filter=Positionals.*` both group by feature.
// =============================================================================

#include <filesystem>
#include <gtest/gtest.h>

// Test the shipped single header, defining the implementation in this TU.
#define CLAP_IMPLEMENTATION
#include "clap.hpp"

#include <string>
#include <vector>

// =============================================================================
// Test toolkit: argv builder, assertion helpers, and the standard app fixture.
// =============================================================================

// Builds a mutable char** argv (as main() receives) from a list of strings.
struct Argv {
    std::vector<std::string> storage;
    std::vector<char*> ptrs;

    Argv(std::initializer_list<std::string> args) {
        storage.assign(args);
        for (auto& s : storage)
            ptrs.push_back(s.data());
    }
    int argc() const { return static_cast<int>(ptrs.size()); }
    char** argv() { return ptrs.data(); }
};

// Parse and assert success (the program should carry on).
static void expect_ok(clap::App& app, Argv& a) {
    EXPECT_TRUE(app.parse(a.argc(), a.argv())) << app.error();
}

// Parse and assert it failed with a specific ErrorKind.
static void expect_error(clap::App& app, Argv& a, clap::ErrorKind kind) {
    ASSERT_FALSE(app.parse(a.argc(), a.argv())) << "expected an error, got none";
    EXPECT_EQ(app.error_kind(), kind);
    EXPECT_FALSE(app.error().empty());
}

// Parse and assert the help flag ended up set. Help is a plain flag, so "help
// was requested" is detected by checking the flag the caller registered.
static void expect_help(clap::App& app, clap::Flag& help, Argv& a) {
    app.parse(a.argc(), a.argv());
    EXPECT_TRUE(help) << "expected the help flag to be set";
}

// The standard app: one argument of every kind, so most tests need no setup.
// Feature sections subclass this (see below) purely to get a feature-named
// GTest suite while sharing this exact configuration.
struct StandardApp : ::testing::Test {
    clap::App app{"prog", "a test program"};
    clap::Flag& help = app.flag("-h,--help", "help");
    clap::Flag& verbose = app.flag("-v,--verbose", "verbose");
    clap::Flag& force = app.flag("-f,--force", "force");
    clap::Option<int>& count = app.option<int>("-c,--count", "count");
    clap::MultiOption<std::string>& names = app.multi_option<std::string>("-n,--names", "names");
    clap::Positional<std::string>& input = app.positional<std::string>("input", "input").default_value("");
};

// Feature suites: same standard app, one GTest suite name per feature so output
// and --gtest_filter group by behaviour. Add a `TEST_F(<suite>, ...)` under the
// matching section to reuse the standard app.
struct LongOptions : StandardApp {};
struct ShortOptions : StandardApp {};
struct Clusters : StandardApp {};
struct Positionals : StandardApp {};
struct MultiOptions : StandardApp {};
struct Values : StandardApp {};
struct HelpFlag : StandardApp {};
struct Errors : StandardApp {};
struct Usage : StandardApp {};
struct Registration : StandardApp {};
struct ParseResult : StandardApp {};

// =============================================================================
// Long options:  --count 10 | --count=10
// =============================================================================

TEST_F(LongOptions, SeparateValue) {
    Argv a{"prog", "--count", "10"};
    expect_ok(app, a);
    EXPECT_EQ(count.get(), 10);
}

TEST_F(LongOptions, EqualsValue) {
    Argv a{"prog", "--count=10"};
    expect_ok(app, a);
    EXPECT_EQ(count.get(), 10);
}

TEST_F(LongOptions, EqualsNegativeValue) {
    Argv a{"prog", "--count=-5"};
    expect_ok(app, a);
    EXPECT_EQ(count.get(), -5);
}

// =============================================================================
// Short options:  -c 10 | -c10 | -c-5
// =============================================================================

TEST_F(ShortOptions, AttachedValue) {
    Argv a{"prog", "-c10"};
    expect_ok(app, a);
    EXPECT_EQ(count.get(), 10);
}

TEST_F(ShortOptions, AttachedNegativeValue) {
    Argv a{"prog", "-c-5"};
    expect_ok(app, a);
    EXPECT_EQ(count.get(), -5);
}

// =============================================================================
// Short clusters:  -vf  (flags combine) | -vc 10  (trailing option takes value)
// =============================================================================

TEST_F(Clusters, FlagsCombine) {
    Argv a{"prog", "-vf"};
    expect_ok(app, a);
    EXPECT_TRUE(verbose);
    EXPECT_TRUE(force);
}

TEST_F(Clusters, TrailingOptionTakesValue) {
    // -vc 10: v is a flag, c is a value-taking option that takes the next token.
    Argv a{"prog", "-vc", "10"};
    expect_ok(app, a);
    EXPECT_TRUE(verbose);
    EXPECT_EQ(count.get(), 10);
}

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

// =============================================================================
// Multi-options:  repeat the flag; never greedy
// =============================================================================

TEST_F(MultiOptions, RepeatsFlag) {
    Argv a{"prog", "-n", "alice", "-n", "bob", "-n", "carol"};
    expect_ok(app, a);
    ASSERT_EQ(names.get().size(), 3u);
    EXPECT_EQ(names.get()[0], "alice");
    EXPECT_EQ(names.get()[1], "bob");
    EXPECT_EQ(names.get()[2], "carol");
}

TEST_F(MultiOptions, NotGreedy) {
    // "-n a b": n takes only "a"; "b" falls through to the positional slot.
    Argv a{"prog", "-n", "a", "b"};
    expect_ok(app, a);
    ASSERT_EQ(names.get().size(), 1u);
    EXPECT_EQ(names.get()[0], "a");
    EXPECT_EQ(input.get(), "b");
}

// =============================================================================
// Value parsing:  typed conversion, and options never eat dash-tokens
// =============================================================================

TEST_F(Values, IntRejectsTrailingGarbage) {
    Argv a{"prog", "--count", "10hey"};
    expect_error(app, a, clap::ErrorKind::InvalidValue);
}

TEST_F(Values, OptionDoesNotEatFollowingFlag) {
    // -c has no value because -v is a flag, not a value -> MissingValue.
    Argv a{"prog", "-c", "-v"};
    expect_error(app, a, clap::ErrorKind::MissingValue);
}

TEST_F(Values, StringOptionAcceptsSpaces) {
    clap::App app{"prog", "d"};
    auto& s = app.option<std::string>("-s,--str", "s");
    Argv a{"prog", "--str", "hello world"};
    expect_ok(app, a);
    EXPECT_EQ(s.get(), "hello world");
}

TEST_F(Values, UnsignedNegativeRejected) {
    clap::App app{"prog", "d"};
    app.option<unsigned>("-u", "unsigned");
    Argv a{"prog", "-u-5"};
    expect_error(app, a, clap::ErrorKind::InvalidValue);
}

TEST_F(Values, LocaleIndependantParsing) {
    try {
        std::locale::global(std::locale("fr_FR.UTF-8"));
    } catch (const std::runtime_error&) {
        GTEST_SKIP() << "fr_FR.UTF-8 locale not installed";
    }
    clap::App app{"prog", "d"};
    app.option<float>("-f", "float");
    Argv a{"prog", "-f", "1,5"};
    expect_error(app, a, clap::ErrorKind::InvalidValue);
}

TEST_F(Values, FilePathOptionAcceptsSpaces) {
    clap::App app{"prog", "d"};
    auto& path = app.option<std::filesystem::path>("-p", "path");
    Argv a{"prog", "-p", "a b c"};
    expect_ok(app, a);
    EXPECT_EQ(path.get(), std::filesystem::path("a b c"));
}

TEST_F(Values, HelpDisplayOfFilepathShowsPath) {
    clap::App app{"prog", "d"};
    app.option<std::filesystem::path>("-p", "path");
    EXPECT_NE(app.help().find("<path>"), std::string::npos);
}

// =============================================================================
// The help flag:  -h/--help is just a flag the caller registers.
//
// parse() walks the whole argv and fills every flag it can, so a caller can
// check help first and let it win over any error that was also recorded.
// =============================================================================

TEST_F(HelpFlag, SetByDashH) {
    Argv a{"prog", "-h"};
    expect_help(app, help, a);
}

TEST_F(HelpFlag, SetInsideCluster) {
    Argv a{"prog", "-vh"};
    expect_help(app, help, a);
}

TEST_F(HelpFlag, SetDespiteEarlierUnknownArgument) {
    Argv a{"prog", "--nope", "-h"};
    expect_help(app, help, a);
}

TEST_F(HelpFlag, SetDespiteEarlierMissingValue) {
    // "-c" swallows nothing (-h is not a value), so -h is still seen as the flag.
    Argv a{"prog", "-c", "-h"};
    expect_help(app, help, a);
}

TEST_F(HelpFlag, HelpAsOptionValueIsNotHelp) {
    // -h here is the *value* of --count, not the help flag.
    Argv a{"prog", "--count=-h"};
    expect_error(app, a, clap::ErrorKind::InvalidValue);
    EXPECT_FALSE(help);
}

// --- custom apps: -h is free unless you register it -------------------------

TEST_F(HelpFlag, SetEvenWhenRequiredMissing) {
    clap::App app{"prog", "d"};
    auto& help = app.flag("-h,--help", "help");
    app.option<int>("-c,--count", "count").required();
    Argv a{"prog", "-h"};
    // parse records the missing-required error, but the help flag is still set,
    // so the caller can check help first and let it win.
    EXPECT_FALSE(app.parse(a.argc(), a.argv()));
    EXPECT_TRUE(help);
}

TEST_F(HelpFlag, DashHFreeWhenNotRegistered) {
    clap::App app{"prog", "d"};
    // nothing auto-registers -h, so it is available for your own use.
    EXPECT_NO_THROW(app.flag("-h,--host", "host"));
}

TEST_F(HelpFlag, DashHIsWhateverYouRegistered) {
    clap::App app{"prog", "d"};
    auto& host = app.flag("-h,--host", "host");
    Argv a{"prog", "-h"};
    expect_ok(app, a);
    EXPECT_TRUE(host);
}

TEST_F(HelpFlag, HelpCanLiveOnAnyName) {
    clap::App app{"prog", "d"};
    auto& help = app.flag("-?,--help", "help");
    Argv a{"prog", "-?"};
    expect_help(app, help, a);
}

// =============================================================================
// Errors:  unknown args, extra positionals, flags rejecting values
// =============================================================================

TEST_F(Errors, UnknownLongOption) {
    Argv a{"prog", "--nope"};
    expect_error(app, a, clap::ErrorKind::UnknownArgument);
}

TEST_F(Errors, ExtraPositionalRejected) {
    Argv a{"prog", "one", "two"};
    expect_error(app, a, clap::ErrorKind::UnknownArgument);
}

TEST_F(Errors, FlagRejectsValue) {
    Argv a{"prog", "--verbose=1"};
    expect_error(app, a, clap::ErrorKind::UnexpectedValue);
}

TEST_F(Errors, MessageCarriesUsageAndCause) {
    Argv a{"prog", "--nope"};
    EXPECT_FALSE(app.parse(a.argc(), a.argv()));
    EXPECT_EQ(app.error_kind(), clap::ErrorKind::UnknownArgument);
    EXPECT_NE(app.error().find(app.usage()), std::string::npos);
    EXPECT_NE(app.error().find("Unknown argument: --nope"), std::string::npos);
}

TEST_F(Errors, FirstErrorIsTheReportedOne) {
    Argv a{"prog", "--nope", "--also-nope"};
    EXPECT_FALSE(app.parse(a.argc(), a.argv()));
    EXPECT_EQ(app.error_kind(), clap::ErrorKind::UnknownArgument);
    EXPECT_NE(app.error().find("Unknown argument: --nope"), std::string::npos);
}

// --- custom app: a required option that is absent ---------------------------

TEST_F(Errors, MissingRequiredReported) {
    clap::App app{"prog", "d"};
    app.option<int>("-c,--count", "count").required();
    Argv a{"prog"};
    expect_error(app, a, clap::ErrorKind::MissingRequiredValue);
}

// =============================================================================
// Usage string
// =============================================================================

TEST_F(Usage, StandardApp) {
    EXPECT_EQ(app.usage(),
        "Usage: prog [-h] [-v] [-f] [-c <int>] [-n <string>]... [<input>]");
}

TEST_F(Usage, RequiredOptionsNotBracketed) {
    clap::App app{"prog", "d"};
    app.option<int>("-c,--count", "count").required();
    app.multi_option<std::string>("-n,--names", "names").required();
    EXPECT_EQ(app.usage(), "Usage: prog -c <int> -n <string>...");
}

TEST_F(Usage, DefaultedPositionalIsBracketed) {
    clap::App app{"prog", "d"};
    app.positional<std::string>("output", "out").default_value("output.txt");
    EXPECT_EQ(app.usage(), "Usage: prog [<output>]");
}

TEST_F(Usage, RequiredPositionalNotBracketed) {
    clap::App app{"prog", "d"};
    app.positional<std::string>("scene", "scene file");
    EXPECT_EQ(app.usage(), "Usage: prog <scene>");
}

// =============================================================================
// Registration:  configuration-time errors (thrown, not parse errors)
// =============================================================================

TEST_F(Registration, NameWithoutDashRejected) {
    clap::App app{"prog", "d"};
    EXPECT_THROW(app.flag("count", "c"), clap::ConfigError);
}

TEST_F(Registration, SingleDashLongNameThrows) {
    clap::App app{"prog", "d"};
    EXPECT_THROW(app.flag("-count", "c"), clap::ConfigError);
}

TEST_F(Registration, DuplicateShortNameRejected) {
    clap::App app{"prog", "d"};
    app.flag("-v,--verbose", "v");
    EXPECT_THROW(app.flag("-v,--victory", "v2"), clap::ConfigError);
}

// =============================================================================
// Parse result + accessor API
// =============================================================================

TEST_F(ParseResult, OkReturnsTrueAndNoError) {
    Argv a{"prog", "-v"};
    EXPECT_TRUE(app.parse(a.argc(), a.argv()));
    EXPECT_TRUE(app.error().empty());
}

TEST_F(ParseResult, HelpIsNotAnErrorAndHelpTextExists) {
    Argv a{"prog", "-h"};
    EXPECT_TRUE(app.parse(a.argc(), a.argv()));  // help is not an error
    EXPECT_TRUE(help);
    EXPECT_FALSE(app.help().empty());
}

TEST_F(ParseResult, GetOnUnsetOptionalThrows) {
    Argv a{"prog"};
    expect_ok(app, a);
    EXPECT_THROW(count.get(), clap::MissingValue);
}

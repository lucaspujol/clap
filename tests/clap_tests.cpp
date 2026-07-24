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
    clap::ValueList<std::string>& names = app.multi_option<std::string>("-n,--names", "names");
    clap::Positional<std::string>& input = app.positional<std::string>("input", "input").default_value("");
};

// Feature suites: same standard app, one GTest suite name per feature so output
// and --gtest_filter group by behaviour. Add a `TEST_F(<suite>, ...)` under the
// matching section to reuse the standard app.
struct Flags        : StandardApp {};
struct LongOptions  : StandardApp {};
struct ShortOptions : StandardApp {};
struct Clusters     : StandardApp {};
struct Positionals  : StandardApp {};
struct MultiOptions : StandardApp {};
struct Variadic     : StandardApp {};
struct Values       : StandardApp {};
struct RangeChoices : StandardApp {};
struct HelpFlag     : StandardApp {};
struct Errors       : StandardApp {};
struct Usage        : StandardApp {};
struct Registration : StandardApp {};
struct ParseResult  : StandardApp {};

// =============================================================================
// Flags:  -v | --verbose
// =============================================================================

TEST_F(Flags, Short) {
    Argv a{"prog", "-v"};
    expect_ok(app, a);
    EXPECT_TRUE(verbose);
}

TEST_F(Flags, ShortIsSet) {
    Argv a{"prog", "-v"};
    expect_ok(app, a);
    EXPECT_TRUE(verbose.is_set());
}

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


// --- custom app: argument both required and default_value -------------------

TEST_F(LongOptions, RequiredAndDefaultValueRejected) {
    clap::App app{"prog", "d"};
    EXPECT_THROW(
        app.option<int>("-c,--count", "count")
            .required()
            .default_value(10),
        clap::ConfigError
    );
}

TEST_F(LongOptions, RequiredAndDefaultValueRejectedInvertedOrder) {
    clap::App app{"prog", "d"};
    EXPECT_THROW(
        app.option<int>("-c,--count", "count")
            .default_value(10)
            .required(),
        clap::ConfigError
    );
}

// --- custom app: default value testing --------------------------------------

TEST_F(LongOptions, DefaultStrCalled) {
    clap::App app{"prog", "d"};
    auto &count = app.option<int>("-c,--count", "count").default_value(10);
    Argv a{"prog", "--count=10"};
    EXPECT_EQ(count.default_str(), "10");
}

TEST_F(LongOptions, GetReturnsDefaultWhenAbsent) {
    clap::App app{"prog", "d"};
    auto &count = app.option<int>("-c,--count", "count").default_value(10);
    Argv a{"prog"};
    EXPECT_EQ(count.get(), 10);
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

TEST_F(MultiOptions, EmptyWhenOptionalAndAbsent) {
    // `names` is optional in the standard app; absent -> empty, get() must not throw.
    Argv a{"prog", "file.txt"};
    expect_ok(app, a);
    EXPECT_TRUE(names.get().empty());
}

TEST_F(MultiOptions, NotGreedy) {
    // "-n a b": n takes only "a"; "b" falls through to the positional slot.
    Argv a{"prog", "-n", "a", "b"};
    expect_ok(app, a);
    ASSERT_EQ(names.get().size(), 1u);
    EXPECT_EQ(names.get()[0], "a");
    EXPECT_EQ(input.get(), "b");
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
// Range & Choices
// =============================================================================

TEST_F(RangeChoices, ChoicesAcceptsListedValue) {
    clap::App app{"prog", "d"};
    auto& fmt = app.option<std::string>("-f,--format", "format")
                    .choices({"json", "xml", "yaml"});
    Argv a{"prog", "-f", "xml"};
    expect_ok(app, a);
    EXPECT_EQ(fmt.get(), "xml");
}

TEST_F(RangeChoices, ChoicesRejectsUnlistedValue) {
    clap::App app{"prog", "d"};
    app.option<std::string>("-f,--format", "format").choices({"json", "xml", "yaml"});
    Argv a{"prog", "-f", "jsn"};
    expect_error(app, a, clap::ErrorKind::InvalidValue);
}

TEST_F(RangeChoices, ChoicesHelpListsAlternatives) {
    clap::App app{"prog", "d"};
    app.option<std::string>("-f,--format", "format").choices({"json", "xml", "yaml"});
    EXPECT_NE(app.help().find("<json|xml|yaml>"), std::string::npos);
}

TEST_F(RangeChoices, ChoicesOnPositionalRejects) {
    clap::App app{"prog", "d"};
    app.positional<std::string>("mode", "mode").choices({"fast", "safe"});
    Argv a{"prog", "turbo"};
    expect_error(app, a, clap::ErrorKind::InvalidValue);
}

TEST_F(RangeChoices, RangeAcceptsValueInside) {
    clap::App app{"prog", "d"};
    auto& jobs = app.option<int>("-j,--jobs", "jobs").range(1, 64);
    Argv a{"prog", "-j", "8"};
    expect_ok(app, a);
    EXPECT_EQ(jobs.get(), 8);
}

TEST_F(RangeChoices, RangeAcceptsInclusiveBound) {
    clap::App app{"prog", "d"};
    auto& jobs = app.option<int>("-j,--jobs", "jobs").range(1, 64);
    Argv a{"prog", "-j", "64"};
    expect_ok(app, a);
    EXPECT_EQ(jobs.get(), 64);
}

TEST_F(RangeChoices, RangeRejectsValueAbove) {
    clap::App app{"prog", "d"};
    app.option<int>("-j,--jobs", "jobs").range(1, 64);
    Argv a{"prog", "-j", "7000"};
    expect_error(app, a, clap::ErrorKind::InvalidValue);
}

TEST_F(RangeChoices, RangeRejectsValueBelow) {
    clap::App app{"prog", "d"};
    app.option<int>("-j,--jobs", "jobs").range(1, 64);
    Argv a{"prog", "-j", "0"};
    expect_error(app, a, clap::ErrorKind::InvalidValue);
}

// range() leans on operator<, so on strings it compares lexicographically.
TEST_F(RangeChoices, RangeWorksLexicographicallyOnStrings) {
    clap::App app{"prog", "d"};
    auto& tier = app.option<std::string>("-t,--tier", "tier").range("a", "m");
    Argv a{"prog", "-t", "gold"};
    expect_ok(app, a);
    EXPECT_EQ(tier.get(), "gold");
}

TEST_F(RangeChoices, RangeRejectsStringOutsideLexRange) {
    clap::App app{"prog", "d"};
    app.option<std::string>("-t,--tier", "tier").range("a", "m");
    Argv a{"prog", "-t", "zeta"};
    expect_error(app, a, clap::ErrorKind::InvalidValue);
}

// On a list the constraint runs per element, not on the collection.
TEST_F(RangeChoices, RangeCheckedPerElementOnVariadic) {
    clap::App app{"prog", "d"};
    app.variadic<int>("ports", "ports").range(1, 65535);
    Argv a{"prog", "22", "8080", "70000"};
    expect_error(app, a, clap::ErrorKind::InvalidValue);
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

TEST_F(Errors, ValueOutOfRange) {
    Argv a{"prog", "--count=7000000000000000"};
    expect_error(app, a, clap::ErrorKind::InvalidValue);
}

// --- custom app: a required option that is absent ---------------------------

TEST_F(Errors, MissingRequiredReported) {
    clap::App app{"prog", "d"};
    app.option<int>("-c,--count", "count").required();
    Argv a{"prog"};
    expect_error(app, a, clap::ErrorKind::MissingRequiredValue);
}

// --- custom app: positional with empty name -------------------------------------

TEST_F(Errors, PositionalWithEmptyNameRejected) {
    clap::App app{"prog", "d"};
    EXPECT_THROW(app.positional<int>("", "pos"), clap::ConfigError);
}

// --- custom app: twice the same positional (duplicate check) --------------------

TEST_F(Errors, DuplicatePositionalRejected) {
    clap::App app{"prog", "d"};
    app.positional<int>("pos", "pos1");
    EXPECT_THROW(app.positional<int>("pos", "pos2"), clap::ConfigError);
}

TEST_F(Errors, OptionalPositionalBeforeRequiredPos) {
    clap::App app{"prog", "d"};
    app.positional<int>("opt", "optional positional arg").default_value(67);
    EXPECT_THROW(app.positional<int>("req", "required positional arg");, clap::ConfigError);
}

// --- custom app: positional name with a comma (splits into two) -----------------

TEST_F(Errors, PositionalWithCommaNameRejected) {
    clap::App app{"prog", "d"};
    EXPECT_THROW(app.positional<int>("a,b", "pos"), clap::ConfigError);
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

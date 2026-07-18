#include <gtest/gtest.h>

// Test the shipped single header, defining the implementation in this TU.
#define CLAP_IMPLEMENTATION
#include "clap.hpp"

#include <string>
#include <vector>

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

// Parse and assert the help flag ended up set (help is a plain flag now, so a
// caller detects "help was requested" by checking the flag it registered).
static void expect_help(clap::App& app, clap::Flag& help, Argv& a) {
    app.parse(a.argc(), a.argv());
    EXPECT_TRUE(help) << "expected the help flag to be set";
}

struct Fixture : ::testing::Test {
    clap::App app{"prog", "a test program"};
    clap::Flag& help = app.flag("-h,--help", "help");
    clap::Flag& verbose = app.flag("-v,--verbose", "verbose");
    clap::Flag& force = app.flag("-f,--force", "force");
    clap::Option<int>& count = app.option<int>("-c,--count", "count");
    clap::MultiOption<std::string>& names = app.multi_option<std::string>("-n,--names", "names");
    clap::Positional<std::string>& input = app.positional<std::string>("input", "input").default_value("");
};

// --- parsing basics -------------------------------------------------------

TEST_F(Fixture, LongOptionWithValue) {
    Argv a{"prog", "--count", "10"};
    expect_ok(app, a);
    EXPECT_EQ(count.get(), 10);
}

TEST_F(Fixture, LongOptionEquals) {
    Argv a{"prog", "--count=10"};
    expect_ok(app, a);
    EXPECT_EQ(count.get(), 10);
}

TEST_F(Fixture, CombinedShortFlags) {
    Argv a{"prog", "-vf"};
    expect_ok(app, a);
    EXPECT_TRUE(verbose);
    EXPECT_TRUE(force);
}

TEST_F(Fixture, PositionalCaptured) {
    Argv a{"prog", "file.txt"};
    expect_ok(app, a);
    EXPECT_EQ(input.get(), "file.txt");
}

// --- step 2: options never eat dash-tokens --------------------------------

TEST_F(Fixture, OptionDoesNotEatFollowingFlag) {
    Argv a{"prog", "-c", "-v"};
    expect_error(app, a, clap::ErrorKind::MissingValue);
}

// --- step 2.5 + string bug: value parsing semantics -----------------------

TEST_F(Fixture, IntOptionRejectsTrailingGarbage) {
    Argv a{"prog", "--count", "10hey"};
    expect_error(app, a, clap::ErrorKind::InvalidValue);
}

TEST_F(Fixture, StringPositionalAcceptsSpaces) {
    Argv a{"prog", "a b c"};
    expect_ok(app, a);
    EXPECT_EQ(input.get(), "a b c");
}

TEST(ParseValue, StringOptionAcceptsSpaces) {
    clap::App app{"prog", "d"};
    auto& s = app.option<std::string>("-s,--str", "s");
    Argv a{"prog", "--str", "hello world"};
    expect_ok(app, a);
    EXPECT_EQ(s.get(), "hello world");
}

// --- step 3: multi-option is repeat-the-flag, not greedy -------------------

TEST_F(Fixture, MultiOptionRepeatsFlag) {
    Argv a{"prog", "-n", "alice", "-n", "bob", "-n", "carol"};
    expect_ok(app, a);
    ASSERT_EQ(names.get().size(), 3u);
    EXPECT_EQ(names.get()[0], "alice");
    EXPECT_EQ(names.get()[1], "bob");
    EXPECT_EQ(names.get()[2], "carol");
}

TEST_F(Fixture, MultiOptionNotGreedy) {
    // "-n a b": n takes only "a"; "b" falls through to the positional slot.
    Argv a{"prog", "-n", "a", "b"};
    expect_ok(app, a);
    ASSERT_EQ(names.get().size(), 1u);
    EXPECT_EQ(names.get()[0], "a");
    EXPECT_EQ(input.get(), "b");
}

// --- step 4: short-attached values + negatives ----------------------------

TEST_F(Fixture, ShortAttachedValue) {
    Argv a{"prog", "-c10"};
    expect_ok(app, a);
    EXPECT_EQ(count.get(), 10);
}

TEST_F(Fixture, ShortAttachedNegative) {
    Argv a{"prog", "-c-5"};
    expect_ok(app, a);
    EXPECT_EQ(count.get(), -5);
}

TEST_F(Fixture, LongEqualsNegative) {
    Argv a{"prog", "--count=-5"};
    expect_ok(app, a);
    EXPECT_EQ(count.get(), -5);
}

TEST_F(Fixture, ClusterThenValueTakingShort) {
    // -vc 10: v is a flag, c takes the next token.
    Argv a{"prog", "-vc", "10"};
    expect_ok(app, a);
    EXPECT_TRUE(verbose);
    EXPECT_EQ(count.get(), 10);
}

// --- positional default_value ---------------------------------------------

TEST(PositionalDefault, UsedWhenAbsent) {
    clap::App app{"prog", "d"};
    auto& out = app.positional<std::string>("output", "out").default_value("output.txt");
    Argv a{"prog"};
    expect_ok(app, a);
    EXPECT_EQ(out.get(), "output.txt");
}

TEST(PositionalDefault, OverriddenWhenPresent) {
    clap::App app{"prog", "d"};
    auto& out = app.positional<std::string>("output", "out").default_value("output.txt");
    Argv a{"prog", "custom.ppm"};
    expect_ok(app, a);
    EXPECT_EQ(out.get(), "custom.ppm");
}

TEST(PositionalDefault, DefaultMakesUsageOptional) {
    clap::App app{"prog", "d"};
    app.positional<std::string>("output", "out").default_value("output.txt");
    EXPECT_EQ(app.usage(), "Usage: prog [<output>]");
}

// --- required positional --------------------------------------------------

TEST(RequiredPositional, MissingReported) {
    clap::App app{"prog", "d"};
    app.positional<std::string>("scene", "scene file");
    Argv a{"prog"};
    expect_error(app, a, clap::ErrorKind::MissingRequiredValue);
}

TEST(RequiredPositional, PresentParses) {
    clap::App app{"prog", "d"};
    auto& scene = app.positional<std::string>("scene", "scene file");
    Argv a{"prog", "scene.txt"};
    expect_ok(app, a);
    EXPECT_EQ(scene.get(), "scene.txt");
}

TEST(RequiredPositional, UsageNotBracketed) {
    clap::App app{"prog", "d"};
    app.positional<std::string>("scene", "scene file");
    EXPECT_EQ(app.usage(), "Usage: prog <scene>");
}

// --- step 5: -h/--help is just a flag the caller registers ----------------

TEST_F(Fixture, HelpFlagSetByDashH) {
    Argv a{"prog", "-h"};
    expect_help(app, help, a);
}

TEST_F(Fixture, HelpInsideClusterWorks) {
    Argv a{"prog", "-vh"};
    expect_help(app, help, a);
}

TEST_F(Fixture, HelpFlagSetEvenWhenRequiredMissing) {
    clap::App app2{"prog", "d"};
    auto& help2 = app2.flag("-h,--help", "help");
    app2.option<int>("-c,--count", "count").required();
    Argv a{"prog", "-h"};
    // parse records the missing-required error, but the help flag is still set,
    // so the caller can check help first and let it win.
    EXPECT_FALSE(app2.parse(a.argc(), a.argv()));
    EXPECT_TRUE(help2);
}

// --- step 6: usage string -------------------------------------------------

TEST_F(Fixture, UsageString) {
    EXPECT_EQ(app.usage(),
        "Usage: prog [-h] [-v] [-f] [-c <int>] [-n <string>]... [<input>]");
}

TEST(Usage, RequiredOptionsNotBracketed) {
    clap::App app{"prog", "d"};
    app.option<int>("-c,--count", "count").required();
    app.multi_option<std::string>("-n,--names", "names").required();
    EXPECT_EQ(app.usage(), "Usage: prog -c <int> -n <string>...");
}

TEST(Registration, NameWithoutDashRejected) {
    clap::App app{"prog", "d"};
    EXPECT_THROW(app.flag("count", "c"), clap::ConfigError);
}

// --- step 7: duplicate-name guard -----------------------------------------

TEST(Registration, DuplicateShortNameRejected) {
    clap::App app{"prog", "d"};
    app.flag("-v,--verbose", "v");
    EXPECT_THROW(app.flag("-v,--victory", "v2"), clap::ConfigError);
}

// --- help is just a flag: -h is free unless you register it ---------------

TEST(HelpFlag, DashHFreeWhenNotRegistered) {
    clap::App app{"prog", "d"};
    // nothing auto-registers -h, so it is available for your own use
    EXPECT_NO_THROW(app.flag("-h,--host", "host"));
}

TEST(HelpFlag, DashHIsWhateverYouRegistered) {
    clap::App app{"prog", "d"};
    auto& host = app.flag("-h,--host", "host");
    Argv a{"prog", "-h"};
    expect_ok(app, a);
    EXPECT_TRUE(host);
}

TEST(HelpFlag, HelpCanLiveOnAnyName) {
    clap::App app{"prog", "d"};
    auto& help = app.flag("-?,--help", "help");
    Argv a{"prog", "-?"};
    expect_help(app, help, a);
}

// --- required / unknown ---------------------------------------------------

TEST(Required, MissingRequiredReported) {
    clap::App app{"prog", "d"};
    app.option<int>("-c,--count", "count").required();
    Argv a{"prog"};
    expect_error(app, a, clap::ErrorKind::MissingRequiredValue);
}

TEST_F(Fixture, UnknownLongOption) {
    Argv a{"prog", "--nope"};
    expect_error(app, a, clap::ErrorKind::UnknownArgument);
}

TEST_F(Fixture, ExtraPositionalRejected) {
    Argv a{"prog", "one", "two"};
    expect_error(app, a, clap::ErrorKind::UnknownArgument);
}

// --- the help flag is set even alongside errors ---------------------------
// parse() walks the whole argv and fills every flag it can, so the caller can
// check help first and let it win over any error that was also recorded.

TEST_F(Fixture, HelpFlagSetDespiteEarlierUnknownArgument) {
    Argv a{"prog", "--nope", "-h"};
    expect_help(app, help, a);
}

TEST_F(Fixture, HelpFlagSetDespiteEarlierMissingValue) {
    // "-c" swallows nothing (-h is not a value), so -h is still seen as the flag
    Argv a{"prog", "-c", "-h"};
    expect_help(app, help, a);
}

TEST_F(Fixture, HelpAsOptionValueIsNotHelp) {
    Argv a{"prog", "--count=-h"};
    expect_error(app, a, clap::ErrorKind::InvalidValue);
    EXPECT_FALSE(help);
}

// --- flags reject values --------------------------------------------------

TEST_F(Fixture, FlagWithValueRejected) {
    Argv a{"prog", "--verbose=1"};
    expect_error(app, a, clap::ErrorKind::UnexpectedValue);
}

// --- parse result + error API ---------------------------------------------

TEST_F(Fixture, OkParseReturnsTrueAndNoError) {
    Argv a{"prog", "-v"};
    EXPECT_TRUE(app.parse(a.argc(), a.argv()));
    EXPECT_TRUE(app.error().empty());
}

TEST_F(Fixture, HelpTextComesFromApp) {
    Argv a{"prog", "-h"};
    EXPECT_TRUE(app.parse(a.argc(), a.argv()));  // help is not an error
    EXPECT_TRUE(help);
    EXPECT_FALSE(app.help().empty());
}

TEST_F(Fixture, ErrorCarriesMessageAndUsage) {
    Argv a{"prog", "--nope"};
    EXPECT_FALSE(app.parse(a.argc(), a.argv()));
    EXPECT_EQ(app.error_kind(), clap::ErrorKind::UnknownArgument);
    EXPECT_NE(app.error().find(app.usage()), std::string::npos);
    EXPECT_NE(app.error().find("Unknown argument: --nope"), std::string::npos);
}

TEST_F(Fixture, FirstErrorIsTheReportedOne) {
    Argv a{"prog", "--nope", "--also-nope"};
    EXPECT_FALSE(app.parse(a.argc(), a.argv()));
    EXPECT_EQ(app.error_kind(), clap::ErrorKind::UnknownArgument);
    EXPECT_NE(app.error().find("Unknown argument: --nope"), std::string::npos);
}

// --- reading an unset optional value is a programmer error ----------------

TEST_F(Fixture, GetOnUnsetOptionalThrows) {
    Argv a{"prog"};
    expect_ok(app, a);
    EXPECT_THROW(count.get(), clap::MissingValue);
}

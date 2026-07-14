#include <gtest/gtest.h>

#include "../src/App.hpp"
#include "../src/ClapExceptions.hpp"

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

struct Fixture : ::testing::Test {
    clap::App app{"prog", "a test program"};
    clap::Flag& verbose = app.flag("-v,--verbose", "verbose");
    clap::Flag& force = app.flag("-f,--force", "force");
    clap::Option<int>& count = app.option<int>("-c,--count", "count");
    clap::MultiOption<std::string>& names = app.multi_option<std::string>("-n,--names", "names");
    clap::Positional<std::string>& input = app.positional<std::string>("input", "input");
};

// --- parsing basics -------------------------------------------------------

TEST_F(Fixture, LongOptionWithValue) {
    Argv a{"prog", "--count", "10"};
    app.parse(a.argc(), a.argv());
    EXPECT_EQ(count.get(), 10);
}

TEST_F(Fixture, LongOptionEquals) {
    Argv a{"prog", "--count=10"};
    app.parse(a.argc(), a.argv());
    EXPECT_EQ(count.get(), 10);
}

TEST_F(Fixture, CombinedShortFlags) {
    Argv a{"prog", "-vf"};
    app.parse(a.argc(), a.argv());
    EXPECT_TRUE(verbose);
    EXPECT_TRUE(force);
}

TEST_F(Fixture, PositionalCaptured) {
    Argv a{"prog", "file.txt"};
    app.parse(a.argc(), a.argv());
    EXPECT_EQ(input.get(), "file.txt");
}

// --- step 2: options never eat dash-tokens --------------------------------

TEST_F(Fixture, OptionDoesNotEatFollowingFlag) {
    Argv a{"prog", "-c", "-v"};
    EXPECT_THROW(app.parse(a.argc(), a.argv()), clap::MissingValue);
}

// --- step 2.5 + string bug: value parsing semantics -----------------------

TEST_F(Fixture, IntOptionRejectsTrailingGarbage) {
    Argv a{"prog", "--count", "10hey"};
    EXPECT_THROW(app.parse(a.argc(), a.argv()), clap::InvalidValue);
}

TEST_F(Fixture, StringPositionalAcceptsSpaces) {
    Argv a{"prog", "a b c"};
    app.parse(a.argc(), a.argv());
    EXPECT_EQ(input.get(), "a b c");
}

TEST(ParseValue, StringOptionAcceptsSpaces) {
    clap::App app{"prog", "d"};
    auto& s = app.option<std::string>("-s,--str", "s");
    Argv a{"prog", "--str", "hello world"};
    app.parse(a.argc(), a.argv());
    EXPECT_EQ(s.get(), "hello world");
}

// --- step 3: multi-option is repeat-the-flag, not greedy -------------------

TEST_F(Fixture, MultiOptionRepeatsFlag) {
    Argv a{"prog", "-n", "alice", "-n", "bob", "-n", "carol"};
    app.parse(a.argc(), a.argv());
    ASSERT_EQ(names.get().size(), 3u);
    EXPECT_EQ(names.get()[0], "alice");
    EXPECT_EQ(names.get()[1], "bob");
    EXPECT_EQ(names.get()[2], "carol");
}

TEST_F(Fixture, MultiOptionNotGreedy) {
    // "-n a b": n takes only "a"; "b" falls through to the positional slot.
    Argv a{"prog", "-n", "a", "b"};
    app.parse(a.argc(), a.argv());
    ASSERT_EQ(names.get().size(), 1u);
    EXPECT_EQ(names.get()[0], "a");
    EXPECT_EQ(input.get(), "b");
}

// --- step 4: short-attached values + negatives ----------------------------

TEST_F(Fixture, ShortAttachedValue) {
    Argv a{"prog", "-c10"};
    app.parse(a.argc(), a.argv());
    EXPECT_EQ(count.get(), 10);
}

TEST_F(Fixture, ShortAttachedNegative) {
    Argv a{"prog", "-c-5"};
    app.parse(a.argc(), a.argv());
    EXPECT_EQ(count.get(), -5);
}

TEST_F(Fixture, LongEqualsNegative) {
    Argv a{"prog", "--count=-5"};
    app.parse(a.argc(), a.argv());
    EXPECT_EQ(count.get(), -5);
}

TEST_F(Fixture, ClusterThenValueTakingShort) {
    // -vc 10: v is a flag, c takes the next token.
    Argv a{"prog", "-vc", "10"};
    app.parse(a.argc(), a.argv());
    EXPECT_TRUE(verbose);
    EXPECT_EQ(count.get(), 10);
}

// --- positional default_value ---------------------------------------------

TEST(PositionalDefault, UsedWhenAbsent) {
    clap::App app{"prog", "d"};
    auto& out = app.positional<std::string>("output", "out").default_value("output.txt");
    Argv a{"prog"};
    app.parse(a.argc(), a.argv());
    EXPECT_EQ(out.get(), "output.txt");
}

TEST(PositionalDefault, OverriddenWhenPresent) {
    clap::App app{"prog", "d"};
    auto& out = app.positional<std::string>("output", "out").default_value("output.txt");
    Argv a{"prog", "custom.ppm"};
    app.parse(a.argc(), a.argv());
    EXPECT_EQ(out.get(), "custom.ppm");
}

TEST(PositionalDefault, RequiredAndDefaultConflict) {
    clap::App app{"prog", "d"};
    auto& p = app.positional<std::string>("x", "x");
    p.required();
    EXPECT_THROW(p.default_value("y"), clap::ConfigError);
}

TEST(PositionalDefault, DefaultMakesUsageOptional) {
    clap::App app{"prog", "d"};
    app.positional<std::string>("output", "out").default_value("output.txt");
    EXPECT_EQ(app.usage(), "Usage: prog [-h] [<output>]");
}

// --- required positional --------------------------------------------------

TEST(RequiredPositional, MissingThrows) {
    clap::App app{"prog", "d"};
    app.positional<std::string>("scene", "scene file").required();
    Argv a{"prog"};
    EXPECT_THROW(app.parse(a.argc(), a.argv()), clap::MissingRequiredArgument);
}

TEST(RequiredPositional, PresentParses) {
    clap::App app{"prog", "d"};
    auto& scene = app.positional<std::string>("scene", "scene file").required();
    Argv a{"prog", "scene.txt"};
    app.parse(a.argc(), a.argv());
    EXPECT_EQ(scene.get(), "scene.txt");
}

TEST(RequiredPositional, UsageNotBracketed) {
    clap::App app{"prog", "d"};
    app.positional<std::string>("scene", "scene file").required();
    EXPECT_EQ(app.usage(), "Usage: prog [-h] <scene>");
}

// --- step 5: -h/--help is a real auto-registered flag ---------------------

TEST_F(Fixture, HelpThrowsHelpRequested) {
    Argv a{"prog", "-h"};
    EXPECT_THROW(app.parse(a.argc(), a.argv()), clap::HelpRequested);
}

TEST_F(Fixture, HelpInsideClusterWorks) {
    Argv a{"prog", "-vh"};
    EXPECT_THROW(app.parse(a.argc(), a.argv()), clap::HelpRequested);
}

TEST_F(Fixture, HelpShortCircuitsRequiredCheck) {
    clap::App app2{"prog", "d"};
    app2.option<int>("-c,--count", "count").required();
    Argv a{"prog", "-h"};
    // help wins even though a required option is missing
    EXPECT_THROW(app2.parse(a.argc(), a.argv()), clap::HelpRequested);
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
    EXPECT_EQ(app.usage(), "Usage: prog [-h] -c <int> -n <string>...");
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

TEST(Registration, HelpNameCollisionRejected) {
    clap::App app{"prog", "d"};
    // -h is auto-registered; re-using it must fail
    EXPECT_THROW(app.flag("-h,--hello", "h"), clap::ConfigError);
}

// --- required / unknown ---------------------------------------------------

TEST(Required, MissingRequiredThrows) {
    clap::App app{"prog", "d"};
    app.option<int>("-c,--count", "count").required();
    Argv a{"prog"};
    EXPECT_THROW(app.parse(a.argc(), a.argv()), clap::MissingRequiredArgument);
}

TEST_F(Fixture, UnknownLongOption) {
    Argv a{"prog", "--nope"};
    EXPECT_THROW(app.parse(a.argc(), a.argv()), clap::UnknownArgument);
}

TEST_F(Fixture, ExtraPositionalRejected) {
    Argv a{"prog", "one", "two"};
    EXPECT_THROW(app.parse(a.argc(), a.argv()), clap::UnknownArgument);
}

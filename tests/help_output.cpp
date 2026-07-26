// Two related things:
//
//   HelpFlag  -h is nothing special. It is a flag the caller registers, and
//             parse() keeps filling flags after an error so help can win.
//   Usage     the generated usage line and the annotations in help text.

#include "support/assertions.hpp"
#include "support/standard_app.hpp"

struct HelpFlag : StandardApp {};
struct Usage    : StandardApp {};

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

TEST_F(Usage, HelpAnnotatesRequired) {
    clap::App app{"prog", "d"};
    app.option<int>("-c,--count", "count").required();
    EXPECT_NE(app.help().find("(required)"), std::string::npos);
}

TEST_F(Usage, HelpAnnotatesDefault) {
    clap::App app{"prog", "d"};
    app.option<int>("-c,--count", "count").default_value(10);
    EXPECT_NE(app.help().find("(default: 10)"), std::string::npos);
}

TEST_F(Usage, HelpAnnotatesEnvFallback) {
    clap::App app{"prog", "d"};
    app.option<int>("-c,--count", "count").from_env("TEST_COUNT");
    EXPECT_NE(app.help().find("(env: TEST_COUNT)"), std::string::npos);
}

// primary_name(), used in usage() is the shortest registered name.
TEST_F(Usage, UsageUsesShortestNameWhateverTheOrder) {
    clap::App app{"prog", "d"};
    app.option<int>("--count,-c", "count");
    EXPECT_NE(app.usage().find("[-c <int>]"), std::string::npos);
}

// Two related things:
//
//   HelpFlag  -h is nothing special. It is a flag the caller registers, and
//             parse() keeps filling flags after an error so help can win.
//   Usage     the generated usage line and the annotations in help text.

#include "support/assertions.hpp"
#include "support/standard_app.hpp"

#include <algorithm>
#include <cstring>
#include <sstream>

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

// What triggers the collapse is the usage running past two lines, not a count
// of options: six one-letter flags fit on one line and stay spelled out.
TEST_F(Usage, ManyShortOptionsDoNotCollapse) {
    clap::App app{"prog", "d"};
    for (const char* name : {"-a", "-b", "-c", "-d", "-e", "-f"})
        app.flag(name, "f");
    EXPECT_EQ(app.usage(), "Usage: prog [-a] [-b] [-c] [-d] [-e] [-f]");
}

TEST_F(Usage, UsagePastTwoLinesCollapses) {
    clap::App app{"prog", "d"};
    for (const char* name : {"--alpha-option", "--beta-option", "--gamma-option",
                             "--delta-option", "--epsilon-option", "--zeta-option",
                             "--eta-option", "--theta-option", "--iota-option",
                             "--kappa-option", "--lambda-option", "--sigma-option"})
        app.flag(name, "f");
    EXPECT_EQ(app.usage(), "Usage: prog [OPTIONS]");
}

// The shape of the command is what survives: required options and positionals.
TEST_F(Usage, CollapseKeepsRequiredOptionsAndPositionals) {
    clap::App app{"prog", "d"};
    for (const char* name : {"--alpha-option", "--beta-option", "--gamma-option",
                             "--delta-option", "--epsilon-option", "--zeta-option",
                             "--eta-option", "--theta-option", "--iota-option",
                             "--kappa-option", "--lambda-option", "--sigma-option"})
        app.flag(name, "f");
    app.option<int>("-j,--jobs", "jobs").required();
    app.positional<std::string>("input", "in");
    EXPECT_EQ(app.usage(), "Usage: prog [OPTIONS] -j <int> <input>");
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

TEST_F(Usage, HelpAnnotatesDefaultList) {
    clap::App app{"prog", "d"};
    app.multi_option<std::string>("-t,--tag", "tags").default_value({"a", "b"});
    EXPECT_NE(app.help().find("(default: a,b)"), std::string::npos);
}

TEST_F(Usage, HelpAnnotatesRequiredOverDefaultOnAList) {
    clap::App app{"prog", "d"};
    app.multi_option<std::string>("-t,--tag", "tags").required();
    EXPECT_NE(app.help().find("(required)"), std::string::npos);
    EXPECT_EQ(app.help().find("(default:"), std::string::npos);
}

TEST_F(Usage, HelpAnnotatesEnvFallback) {
    clap::App app{"prog", "d"};
    app.option<int>("-c,--count", "count").from_env("TEST_COUNT");
    EXPECT_NE(app.help().find("(env: TEST_COUNT)"), std::string::npos);
}

// =============================================================================
// Layout: the widths are measured over the rows that fit, and everything wraps
// at 80 columns rather than letting the terminal do it (#41).
// =============================================================================

namespace {
    // Every line of a block, so a test can assert on the geometry.
    std::vector<std::string> lines_of(const std::string& text) {
        std::vector<std::string> lines;
        std::istringstream stream(text);
        for (std::string line; std::getline(stream, line);)
            lines.push_back(line);
        return lines;
    }

    size_t widest(const std::string& text) {
        size_t max_w = 0;
        for (const auto& line : lines_of(text))
            max_w = std::max(max_w, line.size());
        return max_w;
    }

    // Where the description starts on the line carrying it.
    size_t desc_column_of(const std::string& help, std::string_view needle) {
        for (const auto& line : lines_of(help)) {
            const size_t at = line.find(needle);
            if (at != std::string::npos)
                return at;
        }
        return std::string::npos;
    }
}

TEST_F(Usage, LongDescriptionWrapsAtEighty) {
    clap::App app{"prog", "d"};
    app.option<double>("-r,--retry-backoff",
                       "multiplier applied to the delay between retries when the remote "
                       "returns 5xx or the connection drops midway")
        .default_value(1.5);
    EXPECT_LE(widest(app.help()), 80u);
}

TEST_F(Usage, LongUsageLineWrapsAtEighty) {
    clap::App app{"prog", "d"};
    for (const char* name : {"-a,--alpha", "-b,--beta", "-c,--gamma", "-d,--delta"})
        app.option<std::string>(name, "d").required();
    EXPECT_LE(widest(app.usage()), 80u);
}

TEST_F(Usage, WrappedDescriptionLinesAlignUnderTheFirst) {
    clap::App app{"prog", "d"};
    app.option<int>("-c,--count",
                    "how many times to repeat the operation before it is considered "
                    "finished and the program exits");
    const std::vector<std::string> lines = lines_of(app.help());

    const size_t first = desc_column_of(app.help(), "how many");
    ASSERT_NE(first, std::string::npos);
    for (const auto& line : lines)
        if (line.find("considered") != std::string::npos ||
            line.find("exits") != std::string::npos) {
            EXPECT_EQ(line.find_first_not_of(' '), first);
        }
}

// The point of measuring only the rows that fit: one huge name used to widen
// the column for every other row, which then pushed them all past the cap.
TEST_F(Usage, OneLongNameDoesNotMoveTheOtherDescriptions) {
    clap::App app{"prog", "d"};
    app.option<int>("-c,--count", "how many");
    const size_t before = desc_column_of(app.help(), "how many");

    app.option<int>("-n,--an-absurdly-long-option-name-here", "something else");
    EXPECT_EQ(desc_column_of(app.help(), "how many"), before);
}

// That long row is the one that pays: its description goes on the next line,
// still in the description column.
TEST_F(Usage, OversizedRowPutsItsDescriptionBelow) {
    clap::App app{"prog", "d"};
    app.option<int>("-c,--count", "how many");
    app.option<int>("-n,--an-absurdly-long-option-name-here", "something else");

    for (const auto& line : lines_of(app.help())) {
        if (line.find("--an-absurdly-long") == std::string::npos)
            continue;
        EXPECT_EQ(line.find("something else"), std::string::npos);
    }
    EXPECT_EQ(desc_column_of(app.help(), "something else"),
              desc_column_of(app.help(), "how many"));
}

// Nothing fits, so nothing sizes the column and it falls back to the cap.
TEST_F(Usage, EveryNameTooLongFallsBackToTheCap) {
    clap::App app{"prog", "d"};
    app.option<int>("-a,--an-absurdly-long-option-name-here", "first");
    app.option<int>("-b,--another-absurdly-long-option-name", "second");

    const size_t first = desc_column_of(app.help(), "first");
    EXPECT_NE(first, std::string::npos);
    EXPECT_EQ(desc_column_of(app.help(), "second"), first);
}

TEST_F(Usage, EmptyAppDescriptionIsNotAWrappingError) {
    clap::App app{"prog", ""};
    app.flag("-v,--verbose", "loud");
    EXPECT_NE(app.help().find("-v,--verbose"), std::string::npos);
}

// argv[0] is often a path, and aligning under it would leave no room, so the
// continuation goes under the program name instead.
TEST_F(Usage, LongProgramNameDropsTheAlignedIndent) {
    clap::App app{"./some/deep/directory/tree/with/a/long/prog-name", "d"};
    for (const char* name : {"-a,--alpha", "-b,--beta", "-c,--gamma", "-d,--delta"})
        app.option<std::string>(name, "d").required();

    const std::vector<std::string> lines = lines_of(app.usage());
    ASSERT_GT(lines.size(), 1u);
    EXPECT_EQ(lines[1].find_first_not_of(' '), std::strlen("Usage: "));
}

// primary_name(), used in usage() is the shortest registered name.
TEST_F(Usage, UsageUsesShortestNameWhateverTheOrder) {
    clap::App app{"prog", "d"};
    app.option<int>("--count,-c", "count");
    EXPECT_NE(app.usage().find("[-c <int>]"), std::string::npos);
}

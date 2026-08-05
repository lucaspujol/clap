//   HelpFlag  -h is nothing special. It is a flag the caller registers, and
//             parse() keeps filling flags after an error so help can win.
//   Usage     the generated usage line and the annotations in help text.
//   Examples  the EXAMPLES block built from app.example() pairs.
//   Footer    the free text app.footer() puts at the very end.
//   Hidden    .hidden(), which drops an argument from help but not from parse.
//   Groups    .group(), which splits OPTIONS: into named sections.

#include "support/assertions.hpp"
#include "support/standard_app.hpp"

#include <algorithm>
#include <cstring>
#include <sstream>

struct HelpFlag : StandardApp {};
struct Usage    : StandardApp {};
struct Examples : StandardApp {};
struct Footer   : StandardApp {};
struct Hidden   : StandardApp {};
struct Groups   : StandardApp {};

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

// =============================================================================
// Examples: pairs of (command, description). The description is optional and
// renders as '#' comment lines above the '>' command line, wrapped like the
// rest of the help.
// =============================================================================

namespace {
    // The lines of a help block that start with the given prefix.
    std::vector<std::string> lines_with_prefix(const std::string& help,
                                               std::string_view prefix) {
        std::vector<std::string> found;
        for (const auto& line : lines_of(help))
            if (line.rfind(prefix, 0) == 0)
                found.push_back(line);
        return found;
    }
}

TEST_F(Examples, NoBlockWhenNoneRegistered) {
    EXPECT_EQ(app.help().find("EXAMPLES:"), std::string::npos);
}

TEST_F(Examples, CommandRendersUnderTheBlock) {
    app.example("prog -n alice in.txt");

    const std::string help = app.help();
    EXPECT_NE(help.find("EXAMPLES:"), std::string::npos);
    EXPECT_NE(help.find("  > prog -n alice in.txt"), std::string::npos);
}

TEST_F(Examples, DescriptionPrecedesItsCommand) {
    app.example("prog -n alice in.txt", "the usual case");

    const std::vector<std::string> lines = lines_of(app.help());
    const auto desc = std::find(lines.begin(), lines.end(), "  # the usual case");
    ASSERT_NE(desc, lines.end());
    ASSERT_NE(desc + 1, lines.end());
    EXPECT_EQ(*(desc + 1), "  > prog -n alice in.txt");
}

TEST_F(Examples, DescriptionIsOptional) {
    app.example("prog in.txt");
    EXPECT_TRUE(lines_with_prefix(app.help(), "  # ").empty());
}

// wrap() returns lines without terminators, so the '#' loop has to add its own:
// a description past one line used to come out as a single run-on line.
TEST_F(Examples, LongDescriptionWrapsOntoSeveralCommentLines) {
    app.example("prog in.txt",
                "this description is deliberately very long so that it cannot fit on "
                "one single line of eighty columns and has to wrap");

    const std::vector<std::string> comments = lines_with_prefix(app.help(), "  # ");
    ASSERT_EQ(comments.size(), 2u);
    for (const auto& line : comments)
        EXPECT_LE(line.size(), 80u);
}

// A described example gets a blank line above it so the pairs read apart, but
// only between them: the first one sits right under the header.
TEST_F(Examples, DescribedExamplesAreSeparatedByABlankLine) {
    app.example("prog a", "first");
    app.example("prog b", "second");

    const std::vector<std::string> lines = lines_of(app.help());
    const auto header = std::find(lines.begin(), lines.end(), "EXAMPLES:");
    ASSERT_NE(header, lines.end());
    EXPECT_EQ(*(header + 1), "  # first");
    EXPECT_EQ(*(header + 2), "  > prog a");
    EXPECT_EQ(*(header + 3), "");
    EXPECT_EQ(*(header + 4), "  # second");
}

TEST_F(Examples, EmptyCommandRejected) {
    EXPECT_THROW(app.example(""), clap::ConfigError);
}

// =============================================================================
// Footer: free text printed last, wrapped by default.
// =============================================================================

TEST_F(Footer, PrintsLast) {
    app.footer("see https://example.com");

    const std::vector<std::string> lines = lines_of(app.help());
    ASSERT_FALSE(lines.empty());
    EXPECT_EQ(lines.back(), "see https://example.com");
}

TEST_F(Footer, EmptyFooterPrintsNothing) {
    const std::string before = app.help();
    app.footer("");
    EXPECT_EQ(app.help(), before);
}

// Wrapping is on unless it is turned off. _footer_wrap is a plain bool member,
// so a missing initialiser would make this depend on whatever was in memory.
TEST_F(Footer, WrapsByDefault) {
    app.footer("this footer is deliberately very long so that it cannot fit on one "
               "single line of eighty columns and has to wrap");

    EXPECT_LE(widest(app.help()), 80u);
}

// ASCII art must survive verbatim, which is the whole point of the opt-out.
TEST_F(Footer, DisableFooterWrapKeepsTheTextVerbatim) {
    const std::string art =
        "this footer is deliberately very long so that it cannot fit on one "
        "single line of eighty columns and has to wrap";
    app.disable_footer_wrap();
    app.footer(art);

    const std::vector<std::string> lines = lines_of(app.help());
    EXPECT_EQ(lines.back(), art);
}

// Both are setters read at help() time, so neither order changes the output.
TEST_F(Footer, DisableFooterWrapWorksAfterFooter) {
    const std::string art =
        "this footer is deliberately very long so that it cannot fit on one "
        "single line of eighty columns and has to wrap";
    app.footer(art);
    app.disable_footer_wrap();

    EXPECT_EQ(lines_of(app.help()).back(), art);
}

// =============================================================================
// Hidden: deprecated spellings that still have to work, debug switches. The
// argument parses exactly as it would otherwise; only the printing changes.
//
// HelpFormatter filters once, in its constructor, so hiding cannot leak into
// one of the passes and not the others -- these tests pin each pass anyway.
// =============================================================================

TEST_F(Hidden, OptionIsAbsentFromHelpAndUsage) {
    clap::App app{"prog", "d"};
    app.option<int>("-c,--count", "how many");
    app.option<int>("--legacy-count", "the old spelling").hidden();

    EXPECT_EQ(app.help().find("--legacy-count"), std::string::npos);
    EXPECT_EQ(app.usage().find("--legacy-count"), std::string::npos);
    EXPECT_NE(app.help().find("--count"), std::string::npos);
}

TEST_F(Hidden, FlagStillParses) {
    clap::App app{"prog", "d"};
    auto& debug = app.flag("--debug", "internal").hidden();

    expect_ok(app, {"prog", "--debug"});
    EXPECT_TRUE(debug);
}

TEST_F(Hidden, OptionStillParses) {
    clap::App app{"prog", "d"};
    auto& legacy = app.option<int>("--legacy-count", "the old spelling").hidden();

    expect_ok(app, {"prog", "--legacy-count=7"});
    EXPECT_EQ(legacy.get(), 7);
}

// Hiding is orthogonal to requiredness: it drops out of the usage line but
// check_required() still fires, so a script relying on it keeps its error.
TEST_F(Hidden, RequiredHiddenIsStillEnforced) {
    clap::App app{"prog", "d"};
    app.option<int>("--legacy-count", "the old spelling").hidden().required();

    EXPECT_EQ(app.usage().find("--legacy-count"), std::string::npos);
    expect_error(app, {"prog"}, clap::ErrorKind::MissingRequiredValue);
}

// The positional list is filtered too, and its header is already guarded on
// the list being empty -- which is now emptiable.
TEST_F(Hidden, PositionalIsAbsentButStillTakesItsSlot) {
    clap::App app{"prog", "d"};
    auto& secret = app.positional<std::string>("secret", "internal").hidden();

    EXPECT_EQ(app.help().find("secret"), std::string::npos);
    EXPECT_EQ(app.help().find("POSITIONALS:"), std::string::npos);

    expect_ok(app, {"prog", "value"});
    EXPECT_EQ(secret.get(), "value");
}

// The header would otherwise print with no rows under it.
TEST_F(Hidden, EveryOptionHiddenDropsTheOptionsHeader) {
    clap::App app{"prog", "d"};
    app.flag("--debug", "internal").hidden();

    EXPECT_EQ(app.help().find("OPTIONS:"), std::string::npos);
}

// The reason the filter has to happen before name_width()/desc_column(): a
// hidden name is not printed, so it must not size the columns either.
TEST_F(Hidden, LongHiddenNameDoesNotMoveTheOtherDescriptions) {
    clap::App app{"prog", "d"};
    app.option<int>("-c,--count", "how many");
    const size_t before = desc_column_of(app.help(), "how many");

    app.option<int>("-n,--an-absurdly-long-option-name-here", "something else").hidden();
    EXPECT_EQ(desc_column_of(app.help(), "how many"), before);
}

// =============================================================================
// Groups: .group("Networking") files an option under its own header instead of
// the flat OPTIONS: list. Sections print positionals, then the ungrouped
// options, then the named groups in first-seen order.
//
// Only options group. Positionals are ordered, and that order is the only
// thing their block conveys -- .group() on one is a ConfigError, pinned in
// registration.cpp.
// =============================================================================

namespace {
    // Index of the line that is exactly `header`, for asserting section order.
    size_t line_index(const std::string& help, std::string_view header) {
        const std::vector<std::string> lines = lines_of(help);
        for (size_t i = 0; i < lines.size(); ++i)
            if (lines[i] == header)
                return i;
        return std::string::npos;
    }

    // Index of the first line containing `needle`, for asserting row order
    // without pinning the column widths.
    size_t line_containing(const std::string& help, std::string_view needle) {
        const std::vector<std::string> lines = lines_of(help);
        for (size_t i = 0; i < lines.size(); ++i)
            if (lines[i].find(needle) != std::string::npos)
                return i;
        return std::string::npos;
    }
}

// Nobody calls group(): the output is the flat list it has always been. This
// is the case that must not regress.
TEST_F(Groups, NoGroupCallLeavesOneFlatOptionsBlock) {
    const std::string help = app.help();
    EXPECT_NE(line_index(help, "OPTIONS:"), std::string::npos);
    EXPECT_LT(line_containing(help, "--verbose"), line_containing(help, "--count"));
}

TEST_F(Groups, GroupedOptionMovesUnderItsOwnHeader) {
    clap::App app{"prog", "d"};
    app.flag("-v,--verbose", "loud");
    app.option<int>("-p,--port", "port").group("Networking");

    const std::string help = app.help();
    ASSERT_NE(line_index(help, "Networking:"), std::string::npos);
    EXPECT_GT(line_containing(help, "--port"), line_index(help, "Networking:"));
    EXPECT_LT(line_containing(help, "--verbose"), line_index(help, "Networking:"));
}

// Positionals, then the ungrouped options, then the groups.
TEST_F(Groups, SectionsPrintInOrder) {
    clap::App app{"prog", "d"};
    app.flag("-v,--verbose", "loud");
    app.option<int>("-p,--port", "port").group("Networking");
    app.positional<std::string>("input", "input");

    const std::string help = app.help();
    EXPECT_LT(line_index(help, "POSITIONALS:"), line_index(help, "OPTIONS:"));
    EXPECT_LT(line_index(help, "OPTIONS:"), line_index(help, "Networking:"));
}

// A map would have sorted these; first-seen order is the caller's to choose.
TEST_F(Groups, GroupsKeepFirstSeenOrderNotAlphabetical) {
    clap::App app{"prog", "d"};
    app.option<int>("-z,--zulu", "z").group("Zebra");
    app.option<int>("-a,--alpha", "a").group("Alpha");

    const std::string help = app.help();
    EXPECT_LT(line_index(help, "Zebra:"), line_index(help, "Alpha:"));
}

// Registration order inside a group survives an unrelated option registered
// between the two members.
TEST_F(Groups, ArgumentsKeepRegistrationOrderInsideAGroup) {
    clap::App app{"prog", "d"};
    app.option<int>("--first", "alpha").group("Net");
    app.flag("-v,--verbose", "loud");
    app.option<int>("--second", "beta").group("Net");

    // the descriptions, not the names: the usage line carries both names and
    // would match first.
    const std::string help = app.help();
    EXPECT_LT(line_containing(help, "alpha"), line_containing(help, "beta"));
}

// Whatever the caller typed, including casing and punctuation clap would
// never produce itself.
TEST_F(Groups, HeaderPrintsVerbatim) {
    clap::App app{"prog", "d"};
    app.option<int>("--rpc", "rpc").group("gRPC / debug");

    EXPECT_NE(line_index(app.help(), "gRPC / debug:"), std::string::npos);
}

// Filtering hidden before partitioning, not after: a group whose every member
// is hidden must never be created, or its header prints with no rows.
TEST_F(Groups, GroupOfOnlyHiddenOptionsGetsNoHeader) {
    clap::App app{"prog", "d"};
    app.flag("-v,--verbose", "loud");
    app.option<int>("--secret", "internal").group("Debug").hidden();

    EXPECT_EQ(app.help().find("Debug:"), std::string::npos);
}

// A group with one hidden member and one shown member still prints, without
// the hidden row.
TEST_F(Groups, PartlyHiddenGroupKeepsItsHeader) {
    clap::App app{"prog", "d"};
    app.option<int>("--shown", "shown").group("Debug");
    app.option<int>("--secret", "internal").group("Debug").hidden();

    const std::string help = app.help();
    EXPECT_NE(help.find("Debug:"), std::string::npos);
    EXPECT_EQ(help.find("--secret"), std::string::npos);
}

// The whole reason the widths are measured over the flat option list: per
// group widths would make the descriptions jag between sections.
TEST_F(Groups, DescriptionColumnIsSharedAcrossSections) {
    clap::App app{"prog", "d"};
    app.flag("-v", "loud");
    app.option<int>("--a-fairly-long-name", "grouped").group("Net");

    EXPECT_EQ(desc_column_of(app.help(), "grouped"), desc_column_of(app.help(), "loud"));
}

// Groups are a help-table concern. usage() never sectioned anything and must
// not start now.
TEST_F(Groups, UsageLineIsUnaffected) {
    clap::App app{"prog", "d"};
    app.option<int>("-p,--port", "port").group("Networking");

    const std::string usage = app.usage();
    EXPECT_NE(usage.find("[-p <int>]"), std::string::npos);
    EXPECT_EQ(usage.find("Networking"), std::string::npos);
}

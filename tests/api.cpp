// The shape of the public API around a parse: what parse() returns, what
// error()/help() hold afterwards, and what get() does on an unset argument.

#include "support/assertions.hpp"
#include "support/standard_app.hpp"

struct ParseResult : StandardApp {};

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

// --- get_or(): the non-throwing read ----------------------------------------

TEST_F(ParseResult, GetOrReturnsFallbackWhenUnset) {
    Argv a{"prog"};
    expect_ok(app, a);
    EXPECT_EQ(count.get_or(7), 7);
}

TEST_F(ParseResult, GetOrReturnsParsedValueWhenSet) {
    Argv a{"prog", "-c", "10"};
    expect_ok(app, a);
    EXPECT_EQ(count.get_or(7), 10);
}

TEST_F(ParseResult, GetOrPrefersDefaultOverFallback) {
    // A registered default_value() wins over the fallback passed to get_or().
    clap::App app{"prog", "d"};
    auto& c = app.option<int>("-c,--count", "count").default_value(3);
    Argv a{"prog"};
    expect_ok(app, a);
    EXPECT_EQ(c.get_or(7), 3);
}

TEST_F(ParseResult, GetOrOnStringOption) {
    clap::App app{"prog", "d"};
    auto& s = app.option<std::string>("-s,--str", "s");
    Argv a{"prog"};
    expect_ok(app, a);
    EXPECT_EQ(s.get_or("fallback"), "fallback");
}

// =============================================================================
// parse(std::vector<std::string>)  -  same parser, no char** scaffolding.
// args[0] is the program name and is skipped, exactly as argv[0] is.
// =============================================================================

struct ParseVector : StandardApp {};

TEST_F(ParseVector, ParsesLikeArgv) {
    EXPECT_TRUE(app.parse({"prog", "-v", "-c", "10", "file.txt"}));
    EXPECT_TRUE(verbose);
    EXPECT_EQ(count.get(), 10);
    EXPECT_EQ(input.get(), "file.txt");
}

TEST_F(ParseVector, FirstElementIsTheProgramNameAndIsSkipped) {
    // "-v" in slot 0 is the program name, so the flag stays unset.
    EXPECT_TRUE(app.parse({"-v"}));
    EXPECT_FALSE(verbose);
}

TEST_F(ParseVector, EmptyListIsNotAnError) {
    // No argv[0] at all: nothing to walk, and nothing here is required.
    EXPECT_TRUE(app.parse({}));
    EXPECT_EQ(app.error_kind(), clap::ErrorKind::OK);
}

TEST_F(ParseVector, ReportsErrorsTheSameWay) {
    EXPECT_FALSE(app.parse({"prog", "--nope"}));
    EXPECT_EQ(app.error_kind(), clap::ErrorKind::UnknownArgument);
    EXPECT_FALSE(app.error().empty());
}

TEST_F(ParseVector, ValueWithSpacesNeedsNoQuoting) {
    EXPECT_TRUE(app.parse({"prog", "-n", "a b c"}));
    EXPECT_EQ(names.get(), (std::vector<std::string>{"a b c"}));
}

TEST_F(ParseVector, DoubleDashAndClustersBehaveTheSame) {
    EXPECT_TRUE(app.parse({"prog", "-vf", "--", "-c"}));
    EXPECT_TRUE(verbose);
    EXPECT_TRUE(force);
    EXPECT_EQ(input.get(), "-c");
    EXPECT_FALSE(count.is_set());
}

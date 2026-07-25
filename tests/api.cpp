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

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

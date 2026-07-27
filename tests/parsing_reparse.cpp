// Calling parse() more than once on the same App. Every parse starts over:
// argument values, the positional cursor, "--" mode and the recorded error are
// cleared on entry, so a second argv is parsed as if it were the first. State
// stays readable after parse() returns; only the next parse() wipes it.

#include "support/assertions.hpp"
#include "support/standard_app.hpp"

#include <cstdlib>

struct Reparse : StandardApp {};

TEST_F(Reparse, PositionalSlotStartsOver) {
    Argv first{"prog", "one"};
    expect_ok(app, first);
    EXPECT_EQ(input.get(), "one");

    Argv second{"prog", "two"};
    expect_ok(app, second);
    EXPECT_EQ(input.get(), "two");
}

TEST_F(Reparse, PositionalClearedWhenAbsent) {
    Argv first{"prog", "one"};
    expect_ok(app, first);

    Argv second{"prog"};
    expect_ok(app, second);
    EXPECT_FALSE(input.is_set());
    EXPECT_EQ(input.get(), "");     // falls back to the default again
}

TEST_F(Reparse, FlagClearedWhenAbsent) {
    Argv first{"prog", "-v"};
    expect_ok(app, first);
    EXPECT_TRUE(verbose);

    Argv second{"prog"};
    expect_ok(app, second);
    EXPECT_FALSE(verbose);
}

TEST_F(Reparse, FlagCountDoesNotAccumulate) {
    Argv first{"prog", "-vv"};
    expect_ok(app, first);
    EXPECT_EQ(verbose.count(), 2);

    Argv second{"prog", "-v"};
    expect_ok(app, second);
    EXPECT_EQ(verbose.count(), 1);
}

TEST_F(Reparse, OptionClearedWhenAbsent) {
    Argv first{"prog", "-c", "10"};
    expect_ok(app, first);
    EXPECT_EQ(count.get(), 10);

    Argv second{"prog"};
    expect_ok(app, second);
    EXPECT_FALSE(count.is_set());
    EXPECT_EQ(count.get_or(-1), -1);
}

TEST_F(Reparse, MultiOptionDoesNotAccumulate) {
    Argv first{"prog", "-n", "a", "-n", "b"};
    expect_ok(app, first);
    EXPECT_EQ(names.get().size(), 2u);

    Argv second{"prog", "-n", "c"};
    expect_ok(app, second);
    EXPECT_EQ(names.get(), (std::vector<std::string>{"c"}));
}

TEST_F(Reparse, VariadicPositionalDoesNotAccumulate) {
    clap::App app{"prog", "d"};
    auto& files = app.variadic<std::string>("files", "files");

    Argv first{"prog", "a", "b"};
    expect_ok(app, first);
    EXPECT_EQ(files.get().size(), 2u);

    Argv second{"prog", "c"};
    expect_ok(app, second);
    EXPECT_EQ(files.get(), (std::vector<std::string>{"c"}));
}

// "--" is sticky within one parse; it must not leak into the next one, or the
// second parse would read -v as a positional.
TEST_F(Reparse, PositionalModeCleared) {
    Argv first{"prog", "--", "-v"};
    expect_ok(app, first);
    EXPECT_EQ(input.get(), "-v");
    EXPECT_FALSE(verbose);

    Argv second{"prog", "-v"};
    expect_ok(app, second);
    EXPECT_TRUE(verbose);
    EXPECT_FALSE(input.is_set());
}

TEST_F(Reparse, ErrorClearedAfterSuccess) {
    Argv bad{"prog", "--nope"};
    expect_error(app, bad, clap::ErrorKind::UnknownArgument);

    Argv good{"prog", "-v"};
    expect_ok(app, good);
    EXPECT_TRUE(app.error().empty());
}

// The mirror case: a successful parse must not leave values behind for a parse
// that fails afterwards.
TEST_F(Reparse, ValuesClearedWhenSecondParseFails) {
    Argv good{"prog", "-v", "-c", "10"};
    expect_ok(app, good);

    Argv bad{"prog", "--nope"};
    expect_error(app, bad, clap::ErrorKind::UnknownArgument);
    EXPECT_FALSE(verbose);
    EXPECT_FALSE(count.is_set());
}

TEST_F(Reparse, RequiredCheckedEveryParse) {
    clap::App app{"prog", "d"};
    app.option<int>("-c,--count", "count").required();

    Argv good{"prog", "-c", "1"};
    expect_ok(app, good);

    Argv missing{"prog"};
    expect_error(app, missing, clap::ErrorKind::MissingRequiredValue);
}

// resolve_env() runs on every parse, and only fills what argv left unset. The
// value from the previous parse must not make it skip.
TEST_F(Reparse, EnvResolvedEveryParse) {
    clap::App app{"prog", "d"};
    auto& count = app.option<int>("-c,--count", "count").from_env("TEST_REPARSE_COUNT");
    setenv("TEST_REPARSE_COUNT", "42", 1);

    Argv explicit_value{"prog", "-c", "10"};
    expect_ok(app, explicit_value);
    EXPECT_EQ(count.get(), 10);

    Argv absent{"prog"};
    expect_ok(app, absent);
    EXPECT_EQ(count.get(), 42);
}

// reset() is public: clearing without parsing is allowed.
TEST_F(Reparse, ExplicitResetClearsState) {
    Argv a{"prog", "-v", "-c", "10", "one"};
    expect_ok(app, a);

    app.reset();
    EXPECT_FALSE(verbose);
    EXPECT_FALSE(count.is_set());
    EXPECT_FALSE(input.is_set());
    EXPECT_EQ(app.error_kind(), clap::ErrorKind::OK);
    EXPECT_TRUE(app.error().empty());
}

TEST_F(Reparse, ResetKeepsConfiguration) {
    clap::App app{"prog", "d"};
    auto& count = app.option<int>("-c,--count", "count").default_value(7).choices({7, 9});

    Argv a{"prog", "-c", "9"};
    expect_ok(app, a);
    app.reset();

    EXPECT_EQ(count.get(), 7);              // default survived
    Argv bad{"prog", "-c", "8"};
    expect_error(app, bad, clap::ErrorKind::InvalidValue);   // choices survived
}

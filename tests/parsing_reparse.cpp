// Calling parse() more than once on the same App. Every parse starts over:
// argument values, the positional cursor, "--" mode and the recorded error are
// cleared on entry, so a second argument list is parsed as if it were the
// first. State stays readable after parse() returns; only the next parse()
// wipes it.

#include "support/assertions.hpp"
#include "support/env.hpp"
#include "support/standard_app.hpp"

#include <cstdlib>

struct Reparse : StandardApp {};

TEST_F(Reparse, PositionalSlotStartsOver) {
    expect_ok(app, {"prog", "one"});
    EXPECT_EQ(input.get(), "one");

    expect_ok(app, {"prog", "two"});
    EXPECT_EQ(input.get(), "two");
}

TEST_F(Reparse, PositionalClearedWhenAbsent) {
    expect_ok(app, {"prog", "one"});

    expect_ok(app, {"prog"});
    EXPECT_FALSE(input.is_set());
    EXPECT_EQ(input.get(), "");     // falls back to the default again
}

TEST_F(Reparse, FlagClearedWhenAbsent) {
    expect_ok(app, {"prog", "-v"});
    EXPECT_TRUE(verbose);

    expect_ok(app, {"prog"});
    EXPECT_FALSE(verbose);
}

TEST_F(Reparse, FlagCountDoesNotAccumulate) {
    expect_ok(app, {"prog", "-vv"});
    EXPECT_EQ(verbose.count(), 2);

    expect_ok(app, {"prog", "-v"});
    EXPECT_EQ(verbose.count(), 1);
}

TEST_F(Reparse, OptionClearedWhenAbsent) {
    expect_ok(app, {"prog", "-c", "10"});
    EXPECT_EQ(count.get(), 10);

    expect_ok(app, {"prog"});
    EXPECT_FALSE(count.is_set());
    EXPECT_EQ(count.get_or(-1), -1);
}

TEST_F(Reparse, MultiOptionDoesNotAccumulate) {
    expect_ok(app, {"prog", "-n", "a", "-n", "b"});
    EXPECT_EQ(names.get().size(), 2u);

    expect_ok(app, {"prog", "-n", "c"});
    EXPECT_EQ(names.get(), (std::vector<std::string>{"c"}));
}

TEST_F(Reparse, VariadicPositionalDoesNotAccumulate) {
    clap::App app{"prog", "d"};
    auto& files = app.variadic<std::string>("files", "files");

    expect_ok(app, {"prog", "a", "b"});
    EXPECT_EQ(files.get().size(), 2u);

    expect_ok(app, {"prog", "c"});
    EXPECT_EQ(files.get(), (std::vector<std::string>{"c"}));
}

// "--" is sticky within one parse; it must not leak into the next one, or the
// second parse would read -v as a positional.
TEST_F(Reparse, PositionalModeCleared) {
    expect_ok(app, {"prog", "--", "-v"});
    EXPECT_EQ(input.get(), "-v");
    EXPECT_FALSE(verbose);

    expect_ok(app, {"prog", "-v"});
    EXPECT_TRUE(verbose);
    EXPECT_FALSE(input.is_set());
}

TEST_F(Reparse, ErrorClearedAfterSuccess) {
    expect_error(app, {"prog", "--nope"}, clap::ErrorKind::UnknownArgument);

    expect_ok(app, {"prog", "-v"});
    EXPECT_TRUE(app.error().empty());
}

// The mirror case: a successful parse must not leave values behind for a parse
// that fails afterwards.
TEST_F(Reparse, ValuesClearedWhenSecondParseFails) {
    expect_ok(app, {"prog", "-v", "-c", "10"});

    expect_error(app, {"prog", "--nope"}, clap::ErrorKind::UnknownArgument);
    EXPECT_FALSE(verbose);
    EXPECT_FALSE(count.is_set());
}

TEST_F(Reparse, RequiredCheckedEveryParse) {
    clap::App app{"prog", "d"};
    app.option<int>("-c,--count", "count").required();

    expect_ok(app, {"prog", "-c", "1"});
    expect_error(app, {"prog"}, clap::ErrorKind::MissingRequiredValue);
}

// resolve_env() runs on every parse, and only fills what argv left unset. The
// value from the previous parse must not make it skip.
TEST_F(Reparse, EnvResolvedEveryParse) {
    clap::App app{"prog", "d"};
    auto& count = app.option<int>("-c,--count", "count").from_env("TEST_REPARSE_COUNT");
    setenv("TEST_REPARSE_COUNT", "42", 1);

    expect_ok(app, {"prog", "-c", "10"});
    EXPECT_EQ(count.get(), 10);

    expect_ok(app, {"prog"});
    EXPECT_EQ(count.get(), 42);
}

// reset() is public: clearing without parsing is allowed.
TEST_F(Reparse, ExplicitResetClearsState) {
    expect_ok(app, {"prog", "-v", "-c", "10", "one"});

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

    expect_ok(app, {"prog", "-c", "9"});
    app.reset();

    EXPECT_EQ(count.get(), 7);                                  // default survived
    expect_error(app, {"prog", "-c", "8"}, clap::ErrorKind::InvalidValue);  // choices survived
}

// Mixing entry points: both overloads must reset the same state.
TEST_F(Reparse, ArgvAndVectorOverloadsInterleave) {
    Argv a{"prog", "-v", "-c", "10"};
    expect_ok(app, a);
    EXPECT_TRUE(verbose);

    expect_ok(app, {"prog"});
    EXPECT_FALSE(verbose);
    EXPECT_FALSE(count.is_set());
}

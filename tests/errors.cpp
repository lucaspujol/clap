// Parse-time failures: what argv the parser rejects, which ErrorKind it
// reports, and what the message says. Registration-time failures are thrown
// rather than reported, and live in registration.cpp.

#include "support/assertions.hpp"
#include "support/standard_app.hpp"

struct Errors : StandardApp {};

// --- no error is a state of its own -------------------------------------------

TEST_F(Errors, ErrorKindIsOkBeforeParse) {
    EXPECT_EQ(app.error_kind(), clap::ErrorKind::OK);
    EXPECT_TRUE(app.error().empty());
}

TEST_F(Errors, ErrorKindResetsAfterAFailedParseIsFollowedByAGoodOne) {
    Argv bad{"prog", "--nope"};
    expect_error(app, bad, clap::ErrorKind::UnknownArgument);
    Argv good{"prog", "-v"};
    expect_ok(app, good);
}

TEST_F(Errors, UnknownLongOption) {
    Argv a{"prog", "--nope"};
    expect_error(app, a, clap::ErrorKind::UnknownArgument);
}

TEST_F(Errors, ExtraPositionalRejected) {
    Argv a{"prog", "one", "two"};
    expect_error(app, a, clap::ErrorKind::UnknownArgument);
}

TEST_F(Errors, FlagRejectsValue) {
    Argv a{"prog", "--verbose=1"};
    expect_error(app, a, clap::ErrorKind::UnexpectedValue);
}

TEST_F(Errors, MessageCarriesUsageAndCause) {
    Argv a{"prog", "--nope"};
    EXPECT_FALSE(app.parse(a.argc(), a.argv()));
    EXPECT_EQ(app.error_kind(), clap::ErrorKind::UnknownArgument);
    EXPECT_NE(app.error().find(app.usage()), std::string::npos);
    EXPECT_NE(app.error().find("Unknown argument: --nope"), std::string::npos);
}

// Positionals are parsed after the argv walk, not during it, so "first error
// wins" only stays true because each failure carries the argv slot it came
// from. Both directions have to be checked, or the tagging can be dropped and
// only one of them notices.

TEST_F(Errors, BadPositionalBeatsALaterUnknownOption) {
    clap::App app{"prog", "d"};
    app.option<int>("-c", "count");
    app.positional<int>("num", "number");
    expect_error(app, {"prog", "notanint", "--bogus"}, clap::ErrorKind::InvalidValue);
}

TEST_F(Errors, UnknownOptionBeatsALaterBadPositional) {
    clap::App app{"prog", "d"};
    app.option<int>("-c", "count");
    app.positional<int>("num", "number");
    expect_error(app, {"prog", "--bogus", "notanint"}, clap::ErrorKind::UnknownArgument);
}

TEST_F(Errors, FirstErrorIsTheReportedOne) {
    Argv a{"prog", "--nope", "--also-nope"};
    EXPECT_FALSE(app.parse(a.argc(), a.argv()));
    EXPECT_EQ(app.error_kind(), clap::ErrorKind::UnknownArgument);
    EXPECT_NE(app.error().find("Unknown argument: --nope"), std::string::npos);
}

TEST_F(Errors, ValueOutOfRange) {
    Argv a{"prog", "--count=7000000000000000"};
    expect_error(app, a, clap::ErrorKind::InvalidValue);
}

// --- custom app: a required option that is absent ---------------------------

TEST_F(Errors, MissingRequiredReported) {
    clap::App app{"prog", "d"};
    app.option<int>("-c,--count", "count").required();
    Argv a{"prog"};
    expect_error(app, a, clap::ErrorKind::MissingRequiredValue);
}

// --- custom app: positional with empty name -------------------------------------

TEST_F(Errors, PositionalWithEmptyNameRejected) {
    clap::App app{"prog", "d"};
    EXPECT_THROW(app.positional<int>("", "pos"), clap::ConfigError);
}

// --- custom app: twice the same positional (duplicate check) --------------------

TEST_F(Errors, DuplicatePositionalRejected) {
    clap::App app{"prog", "d"};
    app.positional<int>("pos", "pos1");
    EXPECT_THROW(app.positional<int>("pos", "pos2"), clap::ConfigError);
}

// --- custom app: positional name with a comma (splits into two) -----------------

TEST_F(Errors, PositionalWithCommaNameRejected) {
    clap::App app{"prog", "d"};
    EXPECT_THROW(app.positional<int>("a,b", "pos"), clap::ConfigError);
}

// --- suggestions: an unknown name close to a registered one -------------------

TEST_F(Errors, UnknownArgumentSuggestsCloseName) {
    Argv a{"prog", "--forec"};
    expect_error(app, a, clap::ErrorKind::UnknownArgument);
    EXPECT_NE(app.error().find("did you mean '--force'?"), std::string::npos);
}

TEST_F(Errors, NoSuggestionWhenNothingIsClose) {
    Argv a{"prog", "--wxyzabcdef"};
    expect_error(app, a, clap::ErrorKind::UnknownArgument);
    EXPECT_EQ(app.error().find("did you mean"), std::string::npos);
}

TEST_F(Errors, NoSuggestionWhenEveryNameIsTooShort) {
    // Names under three characters are never compared, so there is no
    // candidate left to suggest at all.
    clap::App app{"prog", "d"};
    app.flag("-v", "v");
    Argv a{"prog", "--verb"};
    expect_error(app, a, clap::ErrorKind::UnknownArgument);
    EXPECT_EQ(app.error().find("did you mean"), std::string::npos);
}

// --- a bare "-" is a name nobody registered -----------------------------------

TEST_F(Errors, BareDashIsUnknown) {
    Argv a{"prog", "-"};
    expect_ok(app, a);
}

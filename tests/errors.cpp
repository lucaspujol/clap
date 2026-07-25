// Parse-time failures: what argv the parser rejects, which ErrorKind it
// reports, and what the message says. Registration-time failures are thrown
// rather than reported, and live in registration.cpp.

#include "support/assertions.hpp"
#include "support/standard_app.hpp"

struct Errors : StandardApp {};

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

TEST_F(Errors, OptionalPositionalBeforeRequiredPos) {
    clap::App app{"prog", "d"};
    app.positional<int>("opt", "optional positional arg").default_value(67);
    EXPECT_THROW(app.positional<int>("req", "required positional arg");, clap::ConfigError);
}

// --- custom app: positional name with a comma (splits into two) -----------------

TEST_F(Errors, PositionalWithCommaNameRejected) {
    clap::App app{"prog", "d"};
    EXPECT_THROW(app.positional<int>("a,b", "pos"), clap::ConfigError);
}

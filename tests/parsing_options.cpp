// Value-taking options, in all the spellings argv can carry them.
//
//   --count 10  --count=10  -c 10  -c10  -c-5
//
// Also the option-level configuration that is rejected or applied at
// registration time: required(), default_value(), and the --/name discard form.

#include "support/assertions.hpp"
#include "support/standard_app.hpp"

struct LongOptions  : StandardApp {};
struct ShortOptions : StandardApp {};

// =============================================================================
// Long options:  --count 10 | --count=10
// =============================================================================

TEST_F(LongOptions, SeparateValue) {
    Argv a{"prog", "--count", "10"};
    expect_ok(app, a);
    EXPECT_EQ(count.get(), 10);
}

TEST_F(LongOptions, EqualsValue) {
    Argv a{"prog", "--count=10"};
    expect_ok(app, a);
    EXPECT_EQ(count.get(), 10);
}

TEST_F(LongOptions, EqualsNegativeValue) {
    Argv a{"prog", "--count=-5"};
    expect_ok(app, a);
    EXPECT_EQ(count.get(), -5);
}


// --- custom app: argument both required and default_value -------------------

TEST_F(LongOptions, RequiredAndDefaultValueRejected) {
    clap::App app{"prog", "d"};
    EXPECT_THROW(
        app.option<int>("-c,--count", "count")
            .required()
            .default_value(10),
        clap::ConfigError
    );
}

TEST_F(LongOptions, RequiredAndDefaultValueRejectedInvertedOrder) {
    clap::App app{"prog", "d"};
    EXPECT_THROW(
        app.option<int>("-c,--count", "count")
            .default_value(10)
            .required(),
        clap::ConfigError
    );
}

// --- custom app: default value testing --------------------------------------

TEST_F(LongOptions, DefaultStrCalled) {
    clap::App app{"prog", "d"};
    auto &count = app.option<int>("-c,--count", "count").default_value(10);
    Argv a{"prog", "--count=10"};
    EXPECT_EQ(count.default_str(), "10");
}

TEST_F(LongOptions, GetReturnsDefaultWhenAbsent) {
    clap::App app{"prog", "d"};
    auto &count = app.option<int>("-c,--count", "count").default_value(10);
    Argv a{"prog"};
    EXPECT_EQ(count.get(), 10);
}

TEST_F(LongOptions, UnknownLongEqualsRejected) {
    Argv a{"prog", "--nope=5"};
    expect_error(app, a, clap::ErrorKind::UnknownArgument);
}

TEST_F(LongOptions, DiscardSlashValidatesButLeavesUnset) {
    // --/count=5: the leading '/' after the dashes validates the option but
    // discards its value, so count stays unset.
    Argv a{"prog", "--/count=5"};
    expect_ok(app, a);
    EXPECT_THROW(count.get(), clap::MissingValue);
}

// =============================================================================
// Short options:  -c 10 | -c10 | -c-5
// =============================================================================

TEST_F(ShortOptions, AttachedValue) {
    Argv a{"prog", "-c10"};
    expect_ok(app, a);
    EXPECT_EQ(count.get(), 10);
}

TEST_F(ShortOptions, AttachedNegativeValue) {
    Argv a{"prog", "-c-5"};
    expect_ok(app, a);
    EXPECT_EQ(count.get(), -5);
}

TEST_F(LongOptions, RequiredOptionSatisfied) {
    clap::App app{"prog", "d"};
    auto& count = app.option<int>("-c,--count", "count").required();
    Argv a{"prog", "--count=7"};
    expect_ok(app, a);
    EXPECT_EQ(count.get(), 7);
}

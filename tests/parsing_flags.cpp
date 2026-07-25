// Flags and short clusters.
//
//   -v            a flag on its own
//   -vf           several flags in one token
//   -vc 10        a cluster ending in a value-taking option

#include "support/assertions.hpp"
#include "support/standard_app.hpp"

struct Flags    : StandardApp {};
struct Clusters : StandardApp {};

// =============================================================================
// Flags:  -v | --verbose
// =============================================================================

TEST_F(Flags, Short) {
    Argv a{"prog", "-v"};
    expect_ok(app, a);
    EXPECT_TRUE(verbose);
}

TEST_F(Flags, ShortIsSet) {
    Argv a{"prog", "-v"};
    expect_ok(app, a);
    EXPECT_TRUE(verbose.is_set());
}

// =============================================================================
// Short clusters:  -vf  (flags combine) | -vc 10  (trailing option takes value)
// =============================================================================

TEST_F(Clusters, FlagsCombine) {
    Argv a{"prog", "-vf"};
    expect_ok(app, a);
    EXPECT_TRUE(verbose);
    EXPECT_TRUE(force);
}

TEST_F(Clusters, TrailingOptionTakesValue) {
    // -vc 10: v is a flag, c is a value-taking option that takes the next token.
    Argv a{"prog", "-vc", "10"};
    expect_ok(app, a);
    EXPECT_TRUE(verbose);
    EXPECT_EQ(count.get(), 10);
}

TEST_F(Clusters, UnknownShortInClusterRejected) {
    // -vq: v is a flag, q matches nothing.
    Argv a{"prog", "-vq"};
    expect_error(app, a, clap::ErrorKind::UnknownArgument);
}

TEST_F(Clusters, TrailingValueOptionWithNoValueErrors) {
    // -vc: c takes a value but the cluster ends and nothing follows.
    Argv a{"prog", "-vc"};
    expect_error(app, a, clap::ErrorKind::MissingValue);
}

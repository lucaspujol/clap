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

// --- count(): a flag remembers how many times it was given -------------------

TEST_F(Flags, CountIsZeroWhenAbsent) {
    Argv a{"prog"};
    expect_ok(app, a);
    EXPECT_EQ(verbose.count(), 0);
    EXPECT_FALSE(verbose);
}

TEST_F(Flags, CountIsOneWhenGivenOnce) {
    Argv a{"prog", "-v"};
    expect_ok(app, a);
    EXPECT_EQ(verbose.count(), 1);
}

TEST_F(Flags, CountsSeparateRepeats) {
    Argv a{"prog", "-v", "-v"};
    expect_ok(app, a);
    EXPECT_EQ(verbose.count(), 2);
    EXPECT_TRUE(verbose);
}

TEST_F(Flags, CountsLongFormRepeats) {
    Argv a{"prog", "--verbose", "--verbose", "--verbose"};
    expect_ok(app, a);
    EXPECT_EQ(verbose.count(), 3);
}

TEST_F(Flags, CountsMixedShortAndLong) {
    Argv a{"prog", "-v", "--verbose"};
    expect_ok(app, a);
    EXPECT_EQ(verbose.count(), 2);
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

// --- count() inside a cluster: -vvv is the whole point of counting -----------

TEST_F(Clusters, RepeatedFlagInClusterCounts) {
    Argv a{"prog", "-vvv"};
    expect_ok(app, a);
    EXPECT_EQ(verbose.count(), 3);
    EXPECT_TRUE(verbose);
}

TEST_F(Clusters, RepeatsAccumulateAcrossClusters) {
    Argv a{"prog", "-vv", "-v"};
    expect_ok(app, a);
    EXPECT_EQ(verbose.count(), 3);
}

TEST_F(Clusters, DistinctFlagsInClusterCountSeparately) {
    Argv a{"prog", "-vvf"};
    expect_ok(app, a);
    EXPECT_EQ(verbose.count(), 2);
    EXPECT_EQ(force.count(), 1);
}

TEST_F(Flags, DiscardSlashLeavesFlagUnset) {
    Argv a{"prog", "-/v"};
    expect_ok(app, a);
    EXPECT_FALSE(verbose);
    EXPECT_EQ(verbose.count(), 0);
}

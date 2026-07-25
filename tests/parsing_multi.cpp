// Repeatable options:  -n a -n b -n c collects into a list.
//
// The thing worth guarding here is that they are never greedy — "-n a b" takes
// only "a", and "b" falls through to the positional slot.

#include "support/assertions.hpp"
#include "support/standard_app.hpp"

struct MultiOptions : StandardApp {};

TEST_F(MultiOptions, RepeatsFlag) {
    Argv a{"prog", "-n", "alice", "-n", "bob", "-n", "carol"};
    expect_ok(app, a);
    ASSERT_EQ(names.get().size(), 3u);
    EXPECT_EQ(names.get()[0], "alice");
    EXPECT_EQ(names.get()[1], "bob");
    EXPECT_EQ(names.get()[2], "carol");
}

TEST_F(MultiOptions, EmptyWhenOptionalAndAbsent) {
    // `names` is optional in the standard app; absent -> empty, get() must not throw.
    Argv a{"prog", "file.txt"};
    expect_ok(app, a);
    EXPECT_TRUE(names.get().empty());
}

TEST_F(MultiOptions, NotGreedy) {
    // "-n a b": n takes only "a"; "b" falls through to the positional slot.
    Argv a{"prog", "-n", "a", "b"};
    expect_ok(app, a);
    ASSERT_EQ(names.get().size(), 1u);
    EXPECT_EQ(names.get()[0], "a");
    EXPECT_EQ(input.get(), "b");
}

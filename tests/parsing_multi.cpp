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

TEST_F(MultiOptions, DiscardSlashValidatesButKeepsNothing) {
    Argv a{"prog", "-/n", "a"};
    expect_ok(app, a);
    EXPECT_TRUE(names.get().empty());
}

// default_value() on a list: the fallback is a whole list, and argv replaces it
// rather than adding to it — same rule as Option, one source wins.

TEST_F(MultiOptions, DefaultListUsedWhenAbsent) {
    clap::App app{"prog", "d"};
    auto& tags = app.multi_option<std::string>("-t,--tag", "tags")
                    .default_value({"a", "b"});
    Argv a{"prog"};
    expect_ok(app, a);
    ASSERT_EQ(tags.get().size(), 2u);
    EXPECT_EQ(tags.get()[0], "a");
    EXPECT_EQ(tags.get()[1], "b");
}

TEST_F(MultiOptions, GivenValuesReplaceDefaultList) {
    clap::App app{"prog", "d"};
    auto& tags = app.multi_option<std::string>("-t,--tag", "tags")
                    .default_value({"a", "b"});
    Argv a{"prog", "-t", "z"};
    expect_ok(app, a);
    ASSERT_EQ(tags.get().size(), 1u);
    EXPECT_EQ(tags.get()[0], "z");
}

TEST_F(MultiOptions, DefaultListLeavesArgumentUnset) {
    // is_set() reports what argv did, not what the default holds — like Option.
    clap::App app{"prog", "d"};
    auto& tags = app.multi_option<std::string>("-t,--tag", "tags")
                    .default_value({"a"});
    Argv a{"prog"};
    expect_ok(app, a);
    EXPECT_FALSE(tags.is_set());
}

TEST_F(MultiOptions, EmptyDefaultListIsTheSameAsNoDefault) {
    clap::App app{"prog", "d"};
    auto& tags = app.multi_option<std::string>("-t,--tag", "tags")
                    .default_value({});
    Argv a{"prog"};
    expect_ok(app, a);
    EXPECT_TRUE(tags.get().empty());
}

TEST_F(MultiOptions, VariadicPositionalTakesADefaultList) {
    clap::App app{"prog", "d"};
    auto& files = app.variadic<std::string>("files", "files")
                     .default_value({"main.cpp"});
    Argv a{"prog"};
    expect_ok(app, a);
    ASSERT_EQ(files.get().size(), 1u);
    EXPECT_EQ(files.get()[0], "main.cpp");
}

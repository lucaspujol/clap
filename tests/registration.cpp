// Configuration-time errors. These are programmer mistakes, not user input, so
// they throw ConfigError at registration instead of being reported by parse().

#include "support/assertions.hpp"
#include "support/standard_app.hpp"

struct Registration : StandardApp {};

TEST_F(Registration, NameWithoutDashRejected) {
    clap::App app{"prog", "d"};
    EXPECT_THROW(app.flag("count", "c"), clap::ConfigError);
}

TEST_F(Registration, SingleDashLongNameThrows) {
    clap::App app{"prog", "d"};
    EXPECT_THROW(app.flag("-count", "c"), clap::ConfigError);
}

TEST_F(Registration, DuplicateShortNameRejected) {
    clap::App app{"prog", "d"};
    app.flag("-v,--verbose", "v");
    EXPECT_THROW(app.flag("-v,--victory", "v2"), clap::ConfigError);
}

TEST_F(Registration, LongNameBadFirstCharRejected) {
    clap::App app{"prog", "d"};
    EXPECT_THROW(app.flag("--@bad", "x"), clap::ConfigError);
}

TEST_F(Registration, LongNameBadInnerCharRejected) {
    clap::App app{"prog", "d"};
    EXPECT_THROW(app.flag("--ab@c", "x"), clap::ConfigError);
}

TEST_F(Registration, EmptyNameRejected) {
    clap::App app{"prog", "d"};
    EXPECT_THROW(app.flag(",", "x"), clap::ConfigError);
}

TEST_F(Registration, BareDashNameRejected) {
    clap::App app{"prog", "d"};
    EXPECT_THROW(app.flag("-", "x"), clap::ConfigError);
}

TEST_F(Registration, SingleCharLongNameRejected) {
    clap::App app{"prog", "d"};
    EXPECT_THROW(app.flag("--x", "x"), clap::ConfigError);
}

TEST_F(Registration, ShortNameWithSpaceRejected) {
    clap::App app{"prog", "d"};
    EXPECT_THROW(app.flag("- ", "x"), clap::ConfigError);
}

TEST_F(Registration, LongNameAcceptsDashAndUnderscore) {
    clap::App app{"prog", "d"};
    EXPECT_NO_THROW(app.flag("--dry-run", "x"));
    EXPECT_NO_THROW(app.flag("--dry_run", "y"));
}

// The same name twice in one registration: the second is unreachable.
TEST_F(Registration, DuplicateNameWithinOneRegistrationRejected) {
    clap::App app{"prog", "d"};
    EXPECT_THROW(app.option<int>("-c,-c", "count"), clap::ConfigError);
    EXPECT_THROW(app.flag("--force,--force", "f"), clap::ConfigError);
}

// '/' is the discard sigil and '=' the long separator, so '-/' and '-=' could
// never be routed to even though they look like valid short names.
TEST_F(Registration, ShortNameWithReservedCharRejected) {
    clap::App app{"prog", "d"};
    EXPECT_THROW(app.option<int>("-/", "x"), clap::ConfigError);
    EXPECT_THROW(app.option<int>("-=", "y"), clap::ConfigError);
}

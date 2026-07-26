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

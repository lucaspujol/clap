// Turning a token into a T: which strings convert, which are rejected, and
// which types are supported at all. Add a test here when adding a ParseValue
// specialisation or a TypeName entry.

#include "support/assertions.hpp"
#include "support/standard_app.hpp"

#include <filesystem>
#include <locale>

struct Values : StandardApp {};

TEST_F(Values, IntRejectsTrailingGarbage) {
    Argv a{"prog", "--count", "10hey"};
    expect_error(app, a, clap::ErrorKind::InvalidValue);
}

TEST_F(Values, OptionDoesNotEatFollowingFlag) {
    // -c has no value because -v is a flag, not a value -> MissingValue.
    Argv a{"prog", "-c", "-v"};
    expect_error(app, a, clap::ErrorKind::MissingValue);
}

TEST_F(Values, StringOptionAcceptsSpaces) {
    clap::App app{"prog", "d"};
    auto& s = app.option<std::string>("-s,--str", "s");
    Argv a{"prog", "--str", "hello world"};
    expect_ok(app, a);
    EXPECT_EQ(s.get(), "hello world");
}

TEST_F(Values, UnsignedNegativeRejected) {
    clap::App app{"prog", "d"};
    app.option<unsigned>("-u", "unsigned");
    Argv a{"prog", "-u-5"};
    expect_error(app, a, clap::ErrorKind::InvalidValue);
}

TEST_F(Values, LocaleIndependantParsing) {
    try {
        std::locale::global(std::locale("fr_FR.UTF-8"));
    } catch (const std::runtime_error&) {
        GTEST_SKIP() << "fr_FR.UTF-8 locale not installed";
    }
    clap::App app{"prog", "d"};
    app.option<float>("-f", "float");
    Argv a{"prog", "-f", "1,5"};
    expect_error(app, a, clap::ErrorKind::InvalidValue);
}

TEST_F(Values, FilePathOptionAcceptsSpaces) {
    clap::App app{"prog", "d"};
    auto& path = app.option<std::filesystem::path>("-p", "path");
    Argv a{"prog", "-p", "a b c"};
    expect_ok(app, a);
    EXPECT_EQ(path.get(), std::filesystem::path("a b c"));
}

TEST_F(Values, HelpDisplayOfFilepathShowsPath) {
    clap::App app{"prog", "d"};
    app.option<std::filesystem::path>("-p", "path");
    EXPECT_NE(app.help().find("<path>"), std::string::npos);
}

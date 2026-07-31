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
    app.option<double>("-f", "double");
    Argv a{"prog", "-f", "1,5"};
    expect_error(app, a, clap::ErrorKind::InvalidValue);
}

TEST_F(Values, FloatAndDoubleDifferOnlyInRange) {
    EXPECT_THROW(clap::ParseValue<float>::parse("1e40"), clap::ParseError);
    EXPECT_DOUBLE_EQ(clap::ParseValue<double>::parse("1e40"), 1e40);
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

// --- double -----------------------------------------------------------------

TEST_F(Values, DoubleParsesFractional) {
    clap::App app{"prog", "d"};
    auto& d = app.option<double>("-d", "double");
    Argv a{"prog", "-d", "1.5"};
    expect_ok(app, a);
    EXPECT_DOUBLE_EQ(d.get(), 1.5);
}

TEST_F(Values, DoubleParsesNegative) {
    clap::App app{"prog", "d"};
    auto& d = app.option<double>("-d", "double");
    Argv a{"prog", "-d-2.25"};
    expect_ok(app, a);
    EXPECT_DOUBLE_EQ(d.get(), -2.25);
}

TEST_F(Values, DoubleRejectsTrailingGarbage) {
    clap::App app{"prog", "d"};
    app.option<double>("-d", "double");
    Argv a{"prog", "-d", "1.5x"};
    expect_error(app, a, clap::ErrorKind::InvalidValue);
}

TEST_F(Values, DoubleHelpShowsDouble) {
    clap::App app{"prog", "d"};
    app.option<double>("-d", "double");
    EXPECT_NE(app.help().find("<double>"), std::string::npos);
}

// --- bool: an option, not a flag. ParseValue<bool> takes named spellings. ----

TEST_F(Values, BoolAcceptsTrueSpellings) {
    for (const char* token : {"1", "true", "yes", "on"}) {
        clap::App app{"prog", "d"};
        auto& b = app.option<bool>("-b", "bool");
        Argv a{"prog", "-b", token};
        expect_ok(app, a);
        EXPECT_TRUE(b.get()) << "token: " << token;
    }
}

TEST_F(Values, BoolAcceptsFalseSpellings) {
    for (const char* token : {"0", "false", "no", "off"}) {
        clap::App app{"prog", "d"};
        auto& b = app.option<bool>("-b", "bool");
        Argv a{"prog", "-b", token};
        expect_ok(app, a);
        EXPECT_FALSE(b.get()) << "token: " << token;
    }
}

TEST_F(Values, BoolRejectsOtherSpellings) {
    for (const char* token : {"True", "YES", "maybe", "2", ""}) {
        clap::App app{"prog", "d"};
        app.option<bool>("-b", "bool");
        Argv a{"prog", "-b", token};
        expect_error(app, a, clap::ErrorKind::InvalidValue);
    }
}

TEST_F(Values, BoolErrorListsAcceptedValues) {
    clap::App app{"prog", "d"};
    app.option<bool>("-b", "bool");
    Argv a{"prog", "-b", "maybe"};
    EXPECT_FALSE(app.parse(a.argc(), a.argv()));
    EXPECT_NE(app.error().find("valid values"), std::string::npos);
}

// A user ParseValue that throws something other than clap::ParseError. The
// obvious thing to reach for is std::runtime_error, and parse() promises never
// to throw on bad input, so it has to become an InvalidValue like any other.
namespace {
    // Deliberately has no comparison operators. Validators are type-erased into
    // std::function, so nothing in the shared path instantiates == or <=, and a
    // custom type only owes what the contract says: TypeName + ParseValue +
    // operator<<. Adding a <=> here would hide a regression rather than fix one.
    struct Boom {
        int v = 0;
    };

    std::ostream& operator<<(std::ostream& os, const Boom& b) { return os << b.v; }
}

namespace clap {
    template<> struct TypeName<Boom> {
        static constexpr std::string_view value = "boom";
    };

    template<> struct ParseValue<Boom> {
        static Boom parse(std::string_view s) {
            if (s == "bad") throw std::runtime_error("user parser exploded");
            return Boom{static_cast<int>(s.size())};
        }
    };
}

TEST_F(Values, UserParserThrowingNonParseErrorBecomesInvalidValue) {
    clap::App app{"prog", "d"};
    app.option<Boom>("-b", "boom");
    Argv a{"prog", "-b", "bad"};
    expect_error(app, a, clap::ErrorKind::InvalidValue);
}

TEST_F(Values, UserParserThrowingNonParseErrorKeepsItsMessage) {
    clap::App app{"prog", "d"};
    app.option<Boom>("-b", "boom");
    Argv a{"prog", "-b", "bad"};
    EXPECT_FALSE(app.parse(a.argc(), a.argv()));
    EXPECT_NE(app.error().find("user parser exploded"), std::string::npos);
}
